#pragma once

#include "helpers.h"

HELPERS_NAMESPACE_BEGIN

#if (defined ARCH_WINDOWS)
	/** 
	 */
	class Win32Handle {
	public:

		HANDLE& operator * () {
			return h_;
		}

		HANDLE* operator -> () {
			return &h_;
		}

		Win32Handle() :
			h_(INVALID_HANDLE_VALUE) {
		}

		Win32Handle(HANDLE h) :
			h_(h) {
		}

		~Win32Handle() {
			close();
		}

		void close() {
			if (h_ != INVALID_HANDLE_VALUE) {
				CloseHandle(h_);
				h_ = INVALID_HANDLE_VALUE;
			}
		}

		operator HANDLE () {
			return h_;
		}

		operator HANDLE* () {
			return &h_;
		}
	private:
		HANDLE h_;
	};
#endif


HELPERS_NAMESPACE_END


