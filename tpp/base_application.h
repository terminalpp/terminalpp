#pragma once 

namespace tpp {

	class BaseApplication {
	public:
		virtual ~BaseApplication() {

		}

		virtual void mainLoop() = 0;

	}; // tpp::Application

} // namespace tpp
