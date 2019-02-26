#pragma once

#include <windows.h>

#include "helpers/helpers.h"

namespace tpp {

	class Win32Error : public helpers::Exception {
	public:
		Win32Error(std::string const & msg):
			helpers::Exception(STR(msg << " - ErrorCode: " << GetLastError())) {
		}
	};

} // namespace tpp