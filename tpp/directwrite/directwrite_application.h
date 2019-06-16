#pragma once
#ifdef WIN32

#include <windows.h>
#include <dwrite_2.h> 
#include <d2d1_2.h>

#include "../application.h"

namespace tpp {
	class DirectWriteApplication : public Application {
	public:
		DirectWriteApplication(HINSTANCE hInstance);

		~DirectWriteApplication() override;

		TerminalWindow* createTerminalWindow(Session* session, TerminalWindow::Properties const& properties, std::string const& name);


	protected:

		void mainLoop() override;

	private:
		friend class DirectWriteTerminalWindow;
		friend class FontSpec<IDWriteTextFormat*>;

		static char const* const TerminalWindowClassName_;

		void registerTerminalWindowClass();


		HINSTANCE hInstance_;

		// Direct write factories that can be used by all windows
		IDWriteFactory* dwFactory_;
		ID2D1Factory* d2dFactory_;
	}; // tpp::DirectWriteApplication 

} // namespace tpp

#endif
