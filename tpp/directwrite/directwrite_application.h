#pragma once
#ifdef WIN32

#include <windows.h>
#include <dwrite_2.h> 
#include <d2d1_2.h>
#include <wrl.h>

#include "../application.h"

namespace tpp {

	class DWriteFont;

	class DirectWriteApplication : public Application {
	public:
		DirectWriteApplication(HINSTANCE hInstance);

		~DirectWriteApplication() override;

		TerminalWindow* createTerminalWindow(Session* session, TerminalWindow::Properties const& properties, std::string const& name);

		std::pair<unsigned, unsigned> terminalCellDimensions(unsigned fontSize) override;

	protected:
		static constexpr WPARAM MSG_TITLE_CHANGE = 0;

		/** New input is available for the attached backend and should be processed in the main event loop.
		 */
		static constexpr WPARAM MSG_INPUT_READY = 1;

		static constexpr WPARAM MSG_BLINK_TIMER = 2;


		void mainLoop() override;

		void sendBlinkTimerMessage() override;


	private:
		friend class DirectWriteTerminalWindow;
		friend class FontSpec<DWriteFont>;

		static char const* const TerminalWindowClassName_;

		void registerTerminalWindowClass();


		HINSTANCE hInstance_;
		DWORD mainLoopThreadId_;

		// Direct write factories that can be used by all windows
		Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory_;
		Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
	}; // tpp::DirectWriteApplication 

	class DWriteFont {
	public:
		Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;
		double sizeEm;
		unsigned ascent;
		DWriteFont(Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace, unsigned sizeEm, unsigned ascent) :
			fontFace(fontFace),
			sizeEm(sizeEm),
			ascent(ascent) {
		}
	};

	template<>
	inline FontSpec<DWriteFont>* FontSpec<DWriteFont>::Create(vterm::Font font, unsigned baseHeight) {
		// get the system font collection		
		Microsoft::WRL::ComPtr<IDWriteFontCollection> sfc;
		Application::Instance<DirectWriteApplication>()->dwFactory_->GetSystemFontCollection(&sfc, false);
		// find the required font family - first get the index then obtain the family by the index
		UINT32 findex;
		BOOL fexists;
		sfc->FindFamilyName(DEFAULT_TERMINAL_FONT, &findex, &fexists);
		Microsoft::WRL::ComPtr<IDWriteFontFamily> ff;
		sfc->GetFontFamily(findex, &ff);
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
		Application::Instance<DirectWriteApplication>()->d2dFactory_->GetDesktopDpi(&dpiX, &dpiY);
		DWRITE_FONT_METRICS metrics;
		fface->GetMetrics(&metrics);
		// the em size is size in pixels divided by (DPI / 96)
		// https://docs.microsoft.com/en-us/windows/desktop/LearnWin32/dpi-and-device-independent-pixels
		double emSize = (baseHeight * font.size()) / (dpiY / 96);
		// we have to adjust this number for the actual font metrics
		emSize = emSize * metrics.designUnitsPerEm / (metrics.ascent + metrics.descent + metrics.lineGap);
		// now we have to determine the height of a character, which we can do via glyph metrics
		DWRITE_GLYPH_METRICS glyphMetrics;
		UINT16 glyph;
		UINT32 codepoint = 'M';
		fface->GetGlyphIndicesA(&codepoint, 1, &glyph);
		fface->GetDesignGlyphMetrics(&glyph, 1, &glyphMetrics);

		return new FontSpec<DWriteFont>(font,
			std::round((static_cast<double>(glyphMetrics.advanceWidth) / glyphMetrics.advanceHeight) * baseHeight * font.size()),
			baseHeight * font.size(),
			DWriteFont(
				fface,
				emSize,
				std::round(emSize * metrics.ascent / metrics.designUnitsPerEm)));
	}

	/** Since DirectWrite stores only the font face and the glyph run then keeps its own size.
	 */
	template<>
	inline vterm::Font FontSpec<DWriteFont>::StripEffects(vterm::Font const& font) {
		vterm::Font result(font);
		result.setBlink(false);
		result.setStrikeout(false);
		result.setUnderline(false);
		return result;
	}



} // namespace tpp

#endif
