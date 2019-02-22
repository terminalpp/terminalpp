#pragma once

#include <string>

namespace tpp {

	/** Settings

	    TODO Elaborate more on how settings are used.
     */
	class SettingsSingleton {
	public:
		std::string fontName = "Iosevka NF";
		unsigned fontHeight = 18;
		unsigned defaultWindowWidth = 640;
		unsigned defaultWindowHeight = 480;
	};

	// defined in main.cpp
	extern SettingsSingleton Settings;


} // namespace tpp