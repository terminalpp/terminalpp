#include <thread>

#include "application.h"

namespace tpp {

	Application* Application::instance_ = nullptr;

	Application::Application() {
		ASSERT(instance_ == nullptr) << "Application must be a singleton";
		instance_ = this;
		// blinking timer
		std::thread t([this]() {
			while (true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				sendBlinkTimerMessage();
			}
		});
		t.detach();
	}

} // namespace tpp