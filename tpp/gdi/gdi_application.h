#pragma once
#ifdef WIN32

#include <windows.h>

#include "../application.h"

namespace tpp {
	class GDIApplication : public Application {
	public:
		GDIApplication(HINSTANCE hInstance);

		~GDIApplication() override;

		TerminalWindow * createTerminalWindow(TerminalWindow::Properties const& properties, std::string const & name);


	protected:

		void mainLoop() override;

	private:
		friend class GDITerminalWindow;

		static char const * const TerminalWindowClassName_;

		void registerTerminalWindowClass();

		
		HINSTANCE hInstance_;
	}; // tpp::GDIApplication 

} // namespace tpp
#endif