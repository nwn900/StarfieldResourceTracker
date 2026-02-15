#include "Settings.h"

namespace ResourceTracker
{
	Settings& Settings::Get()
	{
		static Settings instance;
		return instance;
	}

	void Settings::Load()
	{
		EnsureDefaults();
		addKey   = GetPrivateProfileIntA("Keys", "AddToListKey", 0x42, INI_PATH);
		resetKey = GetPrivateProfileIntA("Keys", "ResetListKey", 0xDC, INI_PATH);
	}

	void Settings::EnsureDefaults()
	{
		for (const char* sub : { "Data", "Data\\SFSE", "Data\\SFSE\\Plugins" })
			CreateDirectoryA(sub, nullptr);

		DWORD attr = GetFileAttributesA(INI_PATH);
		if (attr == INVALID_FILE_ATTRIBUTES)
		{
			WritePrivateProfileStringA("Keys", "AddToListKey",
				"66", INI_PATH);      // 0x42 = 66 = B
			WritePrivateProfileStringA("Keys", "ResetListKey",
				"220", INI_PATH);     // 0xDC = 220 = backslash
		}
	}
}
