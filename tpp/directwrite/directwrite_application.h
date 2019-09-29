#pragma once
#if (defined ARCH_WINDOWS)

#include "windows.h"
#include <dwrite_2.h> 
#include <d2d1_2.h>
#include <wrl.h>

#include "ui/selection.h"

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

        /** Returns the locale name of the current user. 
         */
        wchar_t const * localeName() const {
            return localeName_;
        }

        std::string getSettingsFolder() override;

        Window * createWindow(std::string const & title, int cols, int rows, unsigned cellHeightPx) override;

        void mainLoop() override;

    private:
        friend class DirectWriteWindow;
        friend class DirectWriteFont;
		friend class DirectWriteFont;

        DirectWriteApplication(HINSTANCE hInstance);

        void alert(std::string const & message) override {
            helpers::utf16_string text = helpers::UTF8toUTF16(message);
        	MessageBox(nullptr, text.c_str(), L"t++", MB_ICONEXCLAMATION | MB_TASKMODAL);
        }

        /** Determines whether the WSL is installed or not. 
         
            Returns the default WSL distribution, if any, or empty string if no WSL is present. 
         */
        std::string isWSLPresent() const;

        /** Determines if the ConPTY bypass is present in the WSL or not. 
         */
        bool isBypassPresent() const;

        /** Installs the bypass for given WSL distribution. 
         
            Returns true if the installation was successful, false otherwise. The bypass is installed by downloading the appropriate bypass binary from github releases and installing it into BYPASS_FOLDER (set in config.h).
         */
        bool installBypass(std::string const & wslDistribution);

        void updateDefaultSettings(helpers::JSON & json) override;

		/** Attaches a console to the GDIApplication for debugging purposes.

		    However, launching the bypass pty inside wsl would start its own console unless we allocate console at all times. The trick is to create the console, but then hide the window immediately if we are in a release mode.
		 */
        void attachConsole();

        /** Registers the window class for the application windows. 
         */
        void registerWindowClass();

        /* Handle to the application instance. */
        HINSTANCE hInstance_;

        /* Default locale for the user. */
        wchar_t localeName_[LOCALE_NAME_MAX_LENGTH];

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

        /** Holds the selection so that it can be pasted when requested by the windows. 
         */
        std::string selection_;
        DirectWriteWindow * selectionOwner_;

        static constexpr WPARAM MSG_TITLE_CHANGE = 0;        

		static wchar_t const* const WindowClassName_;

    }; // tpp::DirectWriteApplication

} // namespace tpp

#endif