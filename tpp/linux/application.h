#pragma once
#ifdef __linux__

#include "../base_application.h"

namespace tpp {

	class Application : public BaseApplication {
	public:
		Application();
		~Application() override;

		void mainLoop() override;

	}; // tpp::Application [linux]

}

#endif