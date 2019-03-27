#pragma once
#ifdef WIN32

#include <windows.h>

#include "../base_application.h"

namespace tpp {
	class Application : public BaseApplication {
	public:
		Application(HINSTANCE hInstance);

		~Application() override;

		void mainLoop() override;

	private:
		friend class TerminalWindow;

		static char const * const TerminalWindowClassName_;

		void registerTerminalWindowClass();

		
		HINSTANCE hInstance_;
	};

} // namespace tpp
#endif