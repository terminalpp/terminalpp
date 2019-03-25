#pragma once
#ifdef WIN32

#include <windows.h>

#include "helpers.h"

namespace helpers {

	/** Simple error wrapper which automatically obtains the last Win32 error when thrown.
	 */
	class Win32Error : public helpers::Exception {
	public:
		Win32Error(std::string const & msg) :
			helpers::Exception(STR(msg << " - ErrorCode: " << GetLastError())) {
		}
	};

} // namespace helpers

#endif