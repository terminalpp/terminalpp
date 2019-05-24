#pragma once 

#include <unordered_set>


namespace tpp {

	class BaseTerminalWindow;

	class BaseApplication {
	public:
		virtual ~BaseApplication() {

		}

		virtual void mainLoop() = 0;
	}; // tpp::Application

} // namespace tpp
