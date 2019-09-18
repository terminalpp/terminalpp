#pragma once 
#if (defined ARCH_WINDOWS) 

#include "helpers/string.h"

#include "../config.h"

#include "../font.h"
#include "directwrite_application.h"

namespace tpp {

	class DirectWriteFont : public Font<DirectWriteFont> {
	public:

	    float sizeEm() const {
			return sizeEm_;
		}

		IDWriteFontFace * fontFace() const {
			return fontFace_.Get();
		}

		bool supportsCodepoint(char32_t codepoint) override {
			UINT16 gi;
			UINT32 cp = codepoint;
			fontFace_->GetGlyphIndices(&cp, 1, &gi);
			return (gi != 0);
		}

	private:
	    friend class Font<DirectWriteFont>;

	    class TextAnalysis : public ::Microsoft::WRL::RuntimeClass<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom | ::Microsoft::WRL::InhibitFtmBase>, IDWriteTextAnalysisSource> {
		public:
		    TextAnalysis(char32_t cp) {
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
			virtual HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 textPosition, WCHAR const ** textString, UINT32* textLength) override;
			virtual HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 textPosition, WCHAR const** textString, _Out_ UINT32* textLength) override;
			virtual DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override;
			virtual HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 textPosition, UINT32* textLength, WCHAR const** localeName) override;
			virtual HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 textPosition, UINT32* textLength, IDWriteNumberSubstitution ** numberSubstitution) override;			
		private:
			/* UTF16 encoded codepoint to be analyzed. */
		    wchar_t c_[2];
		};


		DirectWriteFont(ui::Font font, unsigned cellWidth, unsigned cellHeight):
		    Font<DirectWriteFont>(font) {
			DirectWriteApplication * app = DirectWriteApplication::Instance();
			// find the required font family - first get the index then obtain the family by the index
			UINT32 findex;
			BOOL fexists;
			helpers::utf16_string fname = helpers::UTF8toUTF16(font.doubleWidth() ? Config::Instance().doubleWidthFontFamily() : Config::Instance().fontFamily());
			app->systemFontCollection_->FindFamilyName(fname.c_str(), &findex, &fexists);
			if (! fexists) {
				Application::Alert(STR("Unable to load font family " << (font.doubleWidth() ? Config::Instance().doubleWidthFontFamily() : Config::Instance().fontFamily()) << ", trying fallback font (Consolas)"));
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
				font.italics() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL,
				&drw);
			// finally get the font face and initialize the font structures accordingly
			drw->CreateFontFace(&fontFace_);
			cellWidth *= font.width();
			cellHeight *= font.height();
			initializeFromFontFace(cellWidth, cellHeight);
		}

		DirectWriteFont(DirectWriteFont const & from, unsigned cellWidth, unsigned cellHeight, char32_t requiredCodepoint):
		    Font<DirectWriteFont>(from.font_) {
			DirectWriteApplication * app = DirectWriteApplication::Instance();
			DirectWriteFont::TextAnalysis ta(requiredCodepoint);
			UINT32 mappedLength;
			Microsoft::WRL::ComPtr<IDWriteFont> mappedFont;
			FLOAT scale;
			helpers::utf16_string fname = helpers::UTF8toUTF16(font_.doubleWidth() ? Config::Instance().doubleWidthFontFamily() : Config::Instance().fontFamily());
			app->fontFallback_->MapCharacters(
				&ta, // IDWriteTextAnalysisSource
				0, // text position
				1, // text length -- how about surrogate pairs?
				app->systemFontCollection_.Get(), // base font collection
				fname.c_str(), // base family name
				font_.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR, // base weight
				font_.italics() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL, // base style			
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
				fontFace_ = from.fontFace();
			}
			cellWidth *= font_.width();
			cellHeight *= font_.height();
			initializeFromFontFace(cellWidth, cellHeight);
		}

		void initializeFromFontFace(unsigned cellWidth, unsigned cellHeight) {
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
			sizeEm_ = cellHeight / (dpiY / 96);
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
			heightPx_ = cellHeight;
			widthPx_ = static_cast<unsigned>(std::round(static_cast<float>(glyphMetrics.advanceWidth) * sizeEm_ / metrics.designUnitsPerEm));
			// if cellWidth is 0, the centering is ignored (the fontWidth will be used as default cell width)
			if (cellWidth != 0) {
				// if the width is greater then allowed, decrease the font size accordingly and center vertically
				if (widthPx_ > cellWidth) {
					float x = static_cast<float>(cellWidth) / widthPx_;
					sizeEm_ *= x;
					heightPx_ = static_cast<unsigned>(heightPx_ * x);
					widthPx_ = cellWidth;
					offsetTop_ = (cellHeight - heightPx_) / 2;
				// otherwise center horizontally
				} else {
					offsetLeft_ = (cellWidth - widthPx_) / 2;
				}
			}
			// set remaining font properties
			ascent_ = (sizeEm_ * metrics.ascent / metrics.designUnitsPerEm);
			underlineOffset_ = (sizeEm_ * metrics.underlinePosition / metrics.designUnitsPerEm); 
			underlineThickness_ = (sizeEm_ * metrics.underlineThickness / metrics.designUnitsPerEm);
			strikethroughOffset_ = (sizeEm_ * metrics.strikethroughPosition / metrics.designUnitsPerEm);
			strikethroughThickness_ = (sizeEm_ * metrics.strikethroughThickness / metrics.designUnitsPerEm);
		}

		float sizeEm_;
		Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace_;

	};

} // namespace tpp

#endif