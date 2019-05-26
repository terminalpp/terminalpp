#pragma once 

#include <unordered_set>


namespace tpp {

	class Application {
	public:
		virtual ~Application() {

		}

		virtual void mainLoop() = 0;
	}; // tpp::Application

} // namespace tpp
