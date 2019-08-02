#pragma once
#ifdef ARCH_UNIX

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

#undef None

namespace x11 {
	static constexpr long None = 0;
} // namespace X11

#endif
