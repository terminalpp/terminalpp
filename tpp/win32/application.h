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
		HINSTANCE hInstance_;
	};

} // namespace tpp
#endif