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
		unsigned defaultWindowWidth = 720;
		unsigned defaultWindowHeight = 450;
	};

	// defined in main.cpp
	extern SettingsSingleton Settings;


} // namespace tpp