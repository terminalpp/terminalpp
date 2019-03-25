#pragma once
#ifdef WIN32
#include <windows.h>
#include "helpers/win32.h"
#endif

#include "helpers/helpers.h"

namespace tpp {

	/** Terminal++ Application object.

		The application is responsible for maintaining the terminal window objects, settings and so on.

		The tpp header file provides the header for the application and implementation of its platform independent functions, while the platform dependent behavior is implemented in the corresponding platform cpp files.
	 */
	class Application {
	public:

#ifdef WIN32
		Application(HINSTANCE hInstance);
#endif

		~Application();

		void mainLoop();

	private:
#ifdef WIN32
		HINSTANCE hInstance_;
#endif

	};



} // namespace tpp