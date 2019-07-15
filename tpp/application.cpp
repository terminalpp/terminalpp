#include <thread>

#include "application.h"

namespace tpp {

	Application* Application::instance_ = nullptr;

	Application::Application() :
		defaultTerminalWindowProperties_(
			*config::Cols,
			*config::Rows,
			*config::FontSize
		) {
		ASSERT(instance_ == nullptr) << "Application must be a singleton";
		instance_ = this;
	}

	void Application::start() {
		// blinking timer
		std::thread t([this]() {
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000 / (*config::FPS)));
				sendFPSTimerMessage();
			}
			});
		t.detach();

	}

} // namespace tpp