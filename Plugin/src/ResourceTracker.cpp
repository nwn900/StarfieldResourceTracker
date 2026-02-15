#include "ResourceTracker.h"
#include "TrackedResources.h"
#include "Settings.h"

namespace ResourceTracker
{
	static std::atomic<bool> g_running{ false };
	static std::thread       g_inputThread;
	static std::atomic<bool> g_gameReady{ false };

	static std::string VKToName(int vk)
	{
		UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
		char name[64]{};
		if (GetKeyNameTextA(scanCode << 16, name, sizeof(name)) > 0)
			return name;
		return std::format("0x{:02X}", vk);
	}

	static void OnAddKey()
	{
		// Without Address Library we cannot call RE::TESDataHandler::GetSingleton()
		// to iterate recipes.  For now, log the key press.
		// When Address Library format 5 is supported in the build, this will scan
		// all BGSConstructibleObject and BGSResearchProjectForm recipes and add
		// their required component form IDs to the tracked list.
		auto count = TrackedResources::Get().Count();
		INFO("ResourceTracker: [ADD] key pressed  (currently tracking {} resource(s))", count);
		INFO("ResourceTracker: Recipe scanning requires Address Library support (future update)");
	}

	static void OnResetKey()
	{
		auto before = TrackedResources::Get().Count();
		TrackedResources::Get().Clear();
		INFO("ResourceTracker: [RESET] Cleared {} tracked resource(s)", before);
	}

	static void InputThreadFunc()
	{
		auto& settings = Settings::Get();
		bool prevAdd   = false;
		bool prevReset = false;

		while (g_running)
		{
			Sleep(80);

			HWND fg = GetForegroundWindow();
			if (!fg)
				continue;

			char title[256]{};
			GetWindowTextA(fg, title, sizeof(title));
			if (!strstr(title, "Starfield"))
				continue;

			bool curAdd   = (GetAsyncKeyState(settings.addKey)   & 0x8000) != 0;
			bool curReset = (GetAsyncKeyState(settings.resetKey) & 0x8000) != 0;

			if (curAdd && !prevAdd && g_gameReady)
				OnAddKey();

			if (curReset && !prevReset && g_gameReady)
				OnResetKey();

			prevAdd   = curAdd;
			prevReset = curReset;
		}
	}

	void Init()
	{
		Settings::Get().Load();
		TrackedResources::Get().Load();

		auto& s = Settings::Get();
		INFO("ResourceTracker: Initialized  |  Add key: {} ({})  |  Reset key: {} ({})",
			VKToName(s.addKey), s.addKey, VKToName(s.resetKey), s.resetKey);
		INFO("ResourceTracker: Loaded {} tracked resource(s) from file", TrackedResources::Get().Count());
		INFO("ResourceTracker: Press [{}] at a workbench to track missing components", VKToName(s.addKey));
		INFO("ResourceTracker: Press [{}] to reset the tracked list", VKToName(s.resetKey));

		g_running = true;
		g_inputThread = std::thread(InputThreadFunc);
	}

	void Shutdown()
	{
		g_running = false;
		if (g_inputThread.joinable())
			g_inputThread.join();
		TrackedResources::Get().Save();
	}

	void SetGameReady(bool a_ready)
	{
		g_gameReady = a_ready;
	}
}
