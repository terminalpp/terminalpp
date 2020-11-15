#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xrender.h>
#include <fontconfig/fontconfig.h>

#undef None
#undef RootWindow
#undef Success
#undef index
#undef Always

namespace x11 {
	static constexpr long None = 0;

	typedef ::Window Window;
} // namespace X11

#endif
