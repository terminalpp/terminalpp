#pragma once 
#if (defined ARCH_WINDOWS) 

#include "helpers/string.h"

#include "config.h"

#include "../font.h"
#include "directwrite_application.h"

namespace tpp {

	class DirectWriteFont {
	public:
		Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
		float sizeEm;
		DirectWriteFont(Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace, float sizeEm) :
			fontFace{fontFace},
			sizeEm{sizeEm} {
		}

	private:
	    friend class Font<DirectWriteFont>;
	    class TextAnalysis : public ::Microsoft::WRL::RuntimeClass<::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom | ::Microsoft::WRL::InhibitFtmBase>, IDWriteTextAnalysisSource> {
		public:
		    TextAnalysis():
			    c_{0x20,0} {
			}

			/** Sets given codepoint inside the analysis object. 
			 
			    TODO refactor this & helpers::Char::toUTF16 ?
			 */
			void setCodepoint(char32_t cp) {
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
			/* UTF16 encoded codepoint to be analyzed. 
			 */
		    wchar_t c_[2];
		};

	};

	template<>
	inline Font<DirectWriteFont>* Font<DirectWriteFont>::Create(ui::Font font, unsigned cellWidth, unsigned cellHeight) {
		// get the system font collection		
		Microsoft::WRL::ComPtr<IDWriteFontCollection> sfc;
		DirectWriteApplication::Instance()->dwFactory_->GetSystemFontCollection(&sfc, false);
		// find the required font family - first get the index then obtain the family by the index
		UINT32 findex;
		BOOL fexists;
		helpers::utf16_string fname = helpers::UTF8toUTF16(Config::Instance().fontFamily());
		sfc->FindFamilyName(fname.c_str(), &findex, &fexists);
		Microsoft::WRL::ComPtr<IDWriteFontFamily> ff;
		sfc->GetFontFamily(findex, &ff);
		OSCHECK(ff.Get() != nullptr) << "Unable to find font family " << Config::Instance().fontFamily();
		// now get the nearest font
		Microsoft::WRL::ComPtr<IDWriteFont> drw;
		ff->GetFirstMatchingFont(
			font.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STRETCH_NORMAL,
			font.italics() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL,
			&drw);
		// finally get the font face
		Microsoft::WRL::ComPtr<IDWriteFontFace> fface;
		drw->CreateFontFace(&fface);
		// now we need to determine the dimensions of single character, which is relatively involved operation, so first we get the dpi and font metrics
		FLOAT dpiX;
		FLOAT dpiY;
		DirectWriteApplication::Instance()->d2dFactory_->GetDesktopDpi(&dpiX, &dpiY);
		DWRITE_FONT_METRICS metrics;
		fface->GetMetrics(&metrics);
		// the em size is size in pixels divided by (DPI / 96)
		// https://docs.microsoft.com/en-us/windows/desktop/LearnWin32/dpi-and-device-independent-pixels
		// determine the font height in pixels, which is the cell height times the height of the font
		// increase the cell height and width by the font size
		cellHeight = font.calculateHeight(cellHeight);
		cellWidth = font.calculateWidth(cellWidth);
		float emSize = cellHeight / (dpiY / 96);
		// we have to adjust this number for the actual font metrics
		emSize = emSize * metrics.designUnitsPerEm / (metrics.ascent + metrics.descent + metrics.lineGap);
		// now we have to determine the height of a character, which we can do via glyph metrics
		DWRITE_GLYPH_METRICS glyphMetrics;
		UINT16 glyph;
		UINT32 codepoint = 'M';
		fface->GetGlyphIndices(&codepoint, 1, &glyph);
		fface->GetDesignGlyphMetrics(&glyph, 1, &glyphMetrics);
		// get the character dimensions and adjust the font size if necessary so that the characters are centered in their cell area as specified by the ui::Font
		unsigned offsetLeft = 0;
		unsigned offsetTop = 0;
		unsigned fontHeight = cellHeight;
		unsigned fontWidth = static_cast<unsigned>(std::round(static_cast<float>(glyphMetrics.advanceWidth) * emSize / metrics.designUnitsPerEm));
		// if cellWidth is 0, the centering is ignored (the fontWidth will be used as default cell width)
		if (cellWidth != 0) {
			// if the width is greater then allowed, decrease the font size accordingly and center vertically
			if (fontWidth > cellWidth) {
				float x = static_cast<float>(cellWidth) / fontWidth;
				emSize *= x;
				fontHeight = static_cast<unsigned>(fontHeight * x);
				fontWidth = cellWidth;
				offsetTop = (cellHeight - fontHeight) / 2;
			// otherwise center horizontally
			} else {
				offsetLeft = (cellWidth - fontWidth) / 2;
			}
		}
		// create the font
        Font<DirectWriteFont> * result = new Font<DirectWriteFont>(
			font, /* ui font */
			fontWidth,
			fontHeight,
			offsetLeft, 
			offsetTop,
			(emSize * metrics.ascent / metrics.designUnitsPerEm), /* ascent */
			DirectWriteFont(
				fface,
				emSize
			)
		);
		result->underlineOffset_ = (emSize * metrics.underlinePosition / metrics.designUnitsPerEm); 
		result->underlineThickness_ = (emSize * metrics.underlineThickness / metrics.designUnitsPerEm);
		result->strikethroughOffset_ = (emSize * metrics.strikethroughPosition / metrics.designUnitsPerEm);
		result->strikethroughThickness_ = (emSize * metrics.strikethroughThickness / metrics.designUnitsPerEm);
		return result;
	}

	template<>
	inline Font<DirectWriteFont> * Font<DirectWriteFont>::fallbackFor(char32_t character) {
		static DirectWriteFont::TextAnalysis ta;
		static Microsoft::WRL::ComPtr<IDWriteFontFallback> fallback(nullptr);
		Microsoft::WRL::ComPtr<IDWriteFontCollection> sfc;
		// create the font fallback if needed
		if (fallback.Get() == nullptr) {
			OSCHECK(SUCCEEDED(static_cast<IDWriteFactory2*>(DirectWriteApplication::Instance()->dwFactory_.Get())->GetSystemFontFallback(&fallback))) << "Unable to create font fallback";
    		DirectWriteApplication::Instance()->dwFactory_->GetSystemFontCollection(&sfc, false);
		}
		ta.setCodepoint(character);
		UINT32 mappedLength;
		Microsoft::WRL::ComPtr<IDWriteFont> mappedFont;
		FLOAT scale;
		fallback->MapCharacters(
			&ta, // IDWriteTextAnalysisSource
			0, // text position
			1, // text length -- how about surrogate pairs?
			sfc.Get(), // base font collection
			L"Iosevka", // base family name
			font_.bold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR, // base weight
			font_.italics() ? DWRITE_FONT_STYLE_OBLIQUE : DWRITE_FONT_STYLE_NORMAL, // base style			
			DWRITE_FONT_STRETCH_NORMAL, // base stretch
			&mappedLength,
			&mappedFont,
			&scale
		);
		IDWriteFontFamily * matchedFamily;
		IDWriteLocalizedStrings * names;
		mappedFont->GetFontFamily(&matchedFamily);
		matchedFamily->GetFamilyNames(&names);
		Microsoft::WRL::ComPtr<IDWriteFontFace> fface;
		mappedFont->CreateFontFace(&fface);
        Font<DirectWriteFont> * result = new Font<DirectWriteFont>(
			font_, /* ui font */
			widthPx_,
			heightPx_,
			offsetLeft_, 
			offsetTop_,
			ascent_,
			DirectWriteFont(
				fface,
				nativeHandle_.sizeEm * scale
			)
		);
		result->underlineOffset_ = underlineOffset_; 
		result->underlineThickness_ = underlineThickness_;
		result->strikethroughOffset_ = strikethroughOffset_;
		result->strikethroughThickness_ = strikethroughThickness_;
		return result;
	}

	/*
HRESULT MapCharacters(
  IDWriteTextAnalysisSource *analysisSource,
  UINT32                    textPosition,
  UINT32                    textLength,
  IDWriteFontCollection     *baseFontCollection,
  wchar_t const             *baseFamilyName,
  DWRITE_FONT_WEIGHT        baseWeight,
  DWRITE_FONT_STYLE         baseStyle,
  DWRITE_FONT_STRETCH       baseStretch,
  UINT32                    *mappedLength,
  IDWriteFont               **mappedFont,
  FLOAT                     *scale
);

*/


} // namespace tpp

#endif