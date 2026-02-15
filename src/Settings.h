#pragma once

#include <cstdint>

namespace ResourceTracker
{
	struct Settings
	{
		static Settings& Get();

		void Load();
		void EnsureDefaults();

		int addKey    = 0x42;   // VK_B (B key)
		int resetKey  = 0xDC;   // VK_OEM_5 (backslash key)
		int nativeTrackKey = 0x54;  // VK_T (vanilla track for search key)
		bool useNativeTrackBridge = true;

		static constexpr const char* INI_PATH = "Data\\SFSE\\Plugins\\ResourceTracker.ini";
	};
}
