#pragma once
#ifdef WIN32
#include <windows.h>
#endif

#include "helpers/helpers.h"

namespace tpp {

#ifdef WIN32
	/** Simple error wrapper which automatically obtains the last Win32 error when thrown.
	 */
	class Win32Error : public helpers::Exception {
	public:
		Win32Error(std::string const & msg) :
			helpers::Exception(STR(msg << " - ErrorCode: " << GetLastError())) {
		}
	};
#endif

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