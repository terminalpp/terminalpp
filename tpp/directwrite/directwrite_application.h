#pragma once
#if (defined ARCH_WINDOWS)

#include "windows.h"
#include <dwrite_2.h> 
#include <d2d1_2.h>
#include <wrl.h>

#include "ui/clipboard.h"

#include "../application.h"
#include "../font.h"

namespace tpp {

	class DirectWriteFont;

    class DirectWriteApplication : public Application {
    public:

        static void Initialize(HINSTANCE hInstance) {
            new DirectWriteApplication(hInstance);
        }

        static DirectWriteApplication * Instance() {
            return dynamic_cast<DirectWriteApplication*>(Application::Instance());
        }

        Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) override;

        void mainLoop() override;

    private:
        friend class DirectWriteWindow;
		friend class Font<DirectWriteFont>;

        DirectWriteApplication(HINSTANCE hInstance);


		/** Attaches a console to the GDIApplication for debugging purposes.

		    However, launching the bypass pty inside wsl would start its own console unless we allocate console at all times. The trick is to create the console, but then hide the window immediately if we are in a release mode.
		 */
        void attachConsole();

        /** Registers the window class for the application windows. 
         */
        void registerWindowClass();

        /* Handle to the application instance. */
        HINSTANCE hInstance_;

		/* Direct write factories that can be used by all windows, automatically deleted via the WRL pointers */
		Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory_;
		Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;

		static wchar_t const* const WindowClassName_;

        /** Holds the selection so that it can be pasted when requested by the windows. 
         */
        std::string selection_;
        ui::Clipboard * selectionOwner_;

    }; // tpp::DirectWriteApplication

} // namespace tpp

#endif