#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)
#include "directwrite_font.h"

namespace tpp {

    class DirectWriteFont::TextAnalysis : public ::Microsoft::WRL::RuntimeClass<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom | ::Microsoft::WRL::InhibitFtmBase>, IDWriteTextAnalysisSource> {
    public:
        explicit TextAnalysis(char32_t cp) {
            // TODO refactor this & helpers::Char::toUTF16 ?
            if (cp < 0x10000) {
                c_[0] = static_cast<wchar_t>(cp);
            } else {
                cp -= 0x10000;
                unsigned high = cp >> 10; // upper 10 bits
                unsigned low = cp & 0x3ff; // lower 10 bits
                c_[0] = static_cast<wchar_t>(high + 0xd800);
                c_[1] = static_cast<wchar_t>(low + 0xdc00);
            }
        }

        // IDWriteTextAnalysisSource methods
        virtual HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 textPosition, WCHAR const ** textString, UINT32* textLength) override {
            if (textPosition == 0) {
                *textString = c_;
                *textLength = 1; // TODO true for surrogate pairs UTF16?
            } else {
                *textString = nullptr;
                *textLength = 0;
            }
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 textPosition, WCHAR const** textString, _Out_ UINT32* textLength) override {
            ASSERT(textPosition == 0);
            MARK_AS_UNUSED(textPosition);
            *textString = nullptr;  
            *textLength = 0;
            return S_OK;
        }

        virtual DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override {
            // For now only left to right is supported, should the terminal really support RTL?
            return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
        }

        virtual HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 textPosition, UINT32* textLength, WCHAR const** localeName) override {
            ASSERT(textPosition == 0);
            *localeName = DirectWriteApplication::Instance()->localeName_;
            *textLength = 0;
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 textPosition, UINT32* textLength, IDWriteNumberSubstitution ** numberSubstitution) override {
            ASSERT(textPosition == 0);
            *numberSubstitution = nullptr;
            *textLength = 0;
            return S_OK;
        }
    private:
        /* UTF16 encoded codepoint to be analyzed. */
        wchar_t c_[2];

    }; // DirectWriteFont::TextAnalysis

    DirectWriteFont::DirectWriteFont(ui::Font font, int cellHeight, int cellWidth):
        Font<DirectWriteFont>{font, cellHeight, cellWidth} {
        DirectWriteApplication * app = DirectWriteApplication::Instance();
        // find the required font family - first get the index then obtain the family by the index
        UINT32 findex;
        BOOL fexists;
        std::string fnameUTF8{font.doubleWidth() ? tpp::Config::Instance().font.doubleWidthFamily() : tpp::Config::Instance().font.family()};
        helpers::utf16_string fname{helpers::UTF8toUTF16(fnameUTF8)};
        app->systemFontCollection_->FindFamilyName(fname.c_str(), &findex, &fexists);
        if (! fexists) {
            app->alert(STR("Unable to load font family " << fnameUTF8 << ", trying fallback font (Consolas)"));
            app->systemFontCollection_->FindFamilyName(L"Consolas", &findex, &fexists);
            OSCHECK(fexists) << "Unable to initialize fallback font (Consolas)";
        }
        Microsoft::WRL::ComPtr<IDWriteFontFamily> ff;
        app->systemFontCollection_->GetFontFamily(findex, &ff);
        // now get the nearest font
        Microsoft::WRL::ComPtr<IDWriteFont> drw;
        ff->GetFirstMatchingFont(
            font.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STRETCH_NORMAL,
            font.italic() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL,
            &drw);
        // finally get the font face and initialize the font structures accordingly
        drw->CreateFontFace(&fontFace_);
        initializeFromFontFace();
    }

    DirectWriteFont::DirectWriteFont(DirectWriteFont const & base, char32_t codepoint):
        Font<DirectWriteFont>{base.font_, base.cellHeight_, base.cellWidth_} {
        DirectWriteApplication * app = DirectWriteApplication::Instance();
        TextAnalysis ta(codepoint);
        UINT32 mappedLength;
        Microsoft::WRL::ComPtr<IDWriteFont> mappedFont;
        FLOAT scale;
        helpers::utf16_string fname = helpers::UTF8toUTF16(font_.doubleWidth() ? tpp::Config::Instance().font.doubleWidthFamily() : tpp::Config::Instance().font.family());
        app->fontFallback_->MapCharacters(
            &ta, // IDWriteTextAnalysisSource
            0, // text position
            1, // text length -- how about surrogate pairs?
            app->systemFontCollection_.Get(), // base font collection
            fname.c_str(), // base family name
            font_.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR, // base weight
            font_.italic() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL, // base style			
            DWRITE_FONT_STRETCH_NORMAL, // base stretch
            &mappedLength,
            &mappedFont,
            &scale
        );
        if (mappedFont.Get() != nullptr) {
            IDWriteFontFamily * matchedFamily;
            IDWriteLocalizedStrings * names;
            mappedFont->GetFontFamily(&matchedFamily);
            matchedFamily->GetFamilyNames(&names);
            mappedFont->CreateFontFace(&fontFace_);
        } else {
            // no-one implements the character, use the last known font face
            fontFace_ = base.fontFace();
        }
        initializeFromFontFace();
    }

    void DirectWriteFont::initializeFromFontFace() {
        // now we need to determine the dimensions of single character, which is relatively involved operation, so first we get the dpi and font metrics
        FLOAT dpiX;
        FLOAT dpiY;
        DirectWriteApplication::Instance()->d2dFactory_->GetDesktopDpi(&dpiX, &dpiY);
        DWRITE_FONT_METRICS metrics;
        fontFace_->GetMetrics(&metrics);
        // the em size is size in pixels divided by (DPI / 96)
        // https://docs.microsoft.com/en-us/windows/desktop/LearnWin32/dpi-and-device-independent-pixels
        // determine the font height in pixels, which is the cell height times the height of the font
        // increase the cell height and width by the font size
        sizeEm_ = cellHeight_ / (dpiY / 96);
        // we have to adjust this number for the actual font metrics
        sizeEm_ = sizeEm_ * metrics.designUnitsPerEm / (metrics.ascent + metrics.descent + metrics.lineGap);
        // now we have to determine the height of a character, which we can do via glyph metrics
        DWRITE_GLYPH_METRICS glyphMetrics;
        UINT16 glyph;
        UINT32 codepoint = 'M';
        fontFace_->GetGlyphIndices(&codepoint, 1, &glyph);
        fontFace_->GetDesignGlyphMetrics(&glyph, 1, &glyphMetrics);
        // get the character dimensions and adjust the font size if necessary so that the characters are centered in their cell area as specified by the ui::Font
        offsetLeft_ = 0;
        offsetTop_ = 0;
        int h = cellHeight_;
        int w = static_cast<unsigned>(std::round(static_cast<float>(glyphMetrics.advanceWidth) * sizeEm_ / metrics.designUnitsPerEm));
        // if cell width is not specified (0), then the font determines the cell width and no centering is required
        if (cellWidth_ == 0) {
            cellWidth_ = w;
        // if the cell is fully specified, and the font's width is smaller than the width of the cell, the font has to be centered horizontally
        } else if (w <= cellWidth_) {
            offsetLeft_ = (cellWidth_ - w) / 2;
        // otherwise the current size would overflow the cell width so the font has to be scaled down and then centered horizontally
        } else {
            float x = static_cast<float>(cellWidth_) / w;
            sizeEm_ *= x;
            h = static_cast<int>(h * x);
            offsetTop_ = (cellHeight_ - h) / 2;
        }
        // set remaining font properties
        ascent_ = (sizeEm_ * metrics.ascent / metrics.designUnitsPerEm);
        underlineOffset_ = (sizeEm_ * metrics.underlinePosition / metrics.designUnitsPerEm); 
        underlineThickness_ = (sizeEm_ * metrics.underlineThickness / metrics.designUnitsPerEm);
        strikethroughOffset_ = (sizeEm_ * metrics.strikethroughPosition / metrics.designUnitsPerEm);
        strikethroughThickness_ = (sizeEm_ * metrics.strikethroughThickness / metrics.designUnitsPerEm);
    }



} // namespace tpp

#endif