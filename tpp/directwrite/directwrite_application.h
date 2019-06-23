#pragma once
#ifdef WIN32

#include <windows.h>
#include <dwrite_2.h> 
#include <d2d1_2.h>
#include <wrl.h>

#include "../application.h"

namespace tpp {

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

	class DirectWriteApplication : public Application {
	public:
		DirectWriteApplication(HINSTANCE hInstance);

		~DirectWriteApplication() override;

		TerminalWindow* createTerminalWindow(Session* session, TerminalWindow::Properties const& properties, std::string const& name);


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

} // namespace tpp

#endif
