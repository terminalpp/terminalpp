#pragma once
#if (defined ARCH_WINDOWS && defined RENDERER_NATIVE)

#include <windows.h>
#include <windowsx.h>
#include <dwrite_2.h> 
#include <d2d1_2.h>
#include <wrl.h>

#include "../application.h"
#include "../font.h"

namespace tpp {

    class DirectWriteApplication : public Application {
    public:

        static void Initialize(int argc, char ** argv, HINSTANCE hInstance) {
            MARK_AS_UNUSED(argc);
            MARK_AS_UNUSED(argv);
            new DirectWriteApplication(hInstance);
        }

        static DirectWriteApplication * Instance() {
            return dynamic_cast<DirectWriteApplication*>(Application::Instance());
        }

        /** Displays the alert message using Win32 MessageBox API. 
         */
        void alert(std::string const & message) override;

        bool query(std::string const & title, std::string const & message) override;

        /** Opens local file in default viewer/editor
         
            Simply uses the ShellExecute function to perform the default action on given file. If edit is set, it first tries to open the file in edit mode, if that fails due to no association, it tries opening with the default action instead. 
         */
        void openLocalFile(std::string const & filename, bool edit) override;

        void openUrl(std::string const & url) override;

        void setClipboard(std::string const & contents) override;

        Window * createWindow(std::string const & title, int cols, int rows) override;

        void mainLoop() override;


    private:

        friend class DirectWriteWindow;
        friend class DirectWriteFont;

        DirectWriteApplication(HINSTANCE hInstance);

		/** Attaches a console to the GDIApplication for debugging purposes.

		    However, launching the bypass pty inside wsl would start its own console unless we allocate console at all times. The trick is to create the console, but then hide the window immediately if we are in a release mode.
		 */
        void attachConsole();

        /** Registers the window class for the application windows. 
         */
        void registerWindowClass();

        void registerDummyClass();


        /* Default locale for the user. */
        wchar_t localeName_[LOCALE_NAME_MAX_LENGTH];

        HINSTANCE hInstance_;

		/* Direct write factories that can be used by all windows, automatically deleted via the WRL pointers */
		Microsoft::WRL::ComPtr<IDWriteFactory> dwFactory_;
		Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;

        /* Font fallback */
		Microsoft::WRL::ComPtr<IDWriteFontFallback> fontFallback_;
		Microsoft::WRL::ComPtr<IDWriteFontCollection> systemFontCollection_;

        /* Window icons.
         */
        HICON iconDefault_;
        HICON iconNotification_;

        /* Dummy window for scheduling user messages. */
        HWND dummy_;

        /** Holds the selection so that it can be pasted when requested by the windows. 
         */
        std::string selection_;
        DirectWriteWindow * selectionOwner_;


        static LRESULT CALLBACK UserEventHandler_(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        //static constexpr WPARAM MSG_TITLE_CHANGE = 0;   

		static wchar_t const * const WindowClassName_;
        static wchar_t const * const DummyWindowName_;

    }; // DirectWriteApplication



} // namespace tpp

#endif