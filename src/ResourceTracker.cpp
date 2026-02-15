#include "ResourceTracker.h"
#include "Settings.h"
#include "TrackedResources.h"

namespace ResourceTracker
{
	static std::atomic<bool> g_running{ false };
	static std::thread       g_inputThread;
	static std::atomic<bool> g_gameReady{ false };

	static std::string VKToName(int vk)
	{
		UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
		char name[64]{};
		if (GetKeyNameTextA(scanCode << 16, name, sizeof(name)) > 0) {
			return name;
		}
		return std::format("0x{:02X}", vk);
	}

	template <class TEvent>
	static bool DispatchUIEvent()
	{
		auto* source = TEvent::GetEventSource();
		if (!source) {
			return false;
		}

		TEvent event{};
		source->Notify(&event);
		return true;
	}

	static void OnAddKey()
	{
		bool fired = false;

		if (DispatchUIEvent<RE::ResearchMenu_ToggleTrackingProject>()) {
			spdlog::info("ResourceTracker: B -> ResearchMenu_ToggleTrackingProject");
			fired = true;
		}

		if (DispatchUIEvent<RE::CraftingMenu_ToggleTracking>()) {
			spdlog::info("ResourceTracker: B -> CraftingMenu_ToggleTracking");
			fired = true;
		}

		if (!fired) {
			spdlog::info("ResourceTracker: B pressed, but no tracking event source was available");
		}
	}

	static void OnResetKey()
	{
		auto before = TrackedResources::Get().Count();
		TrackedResources::Get().Clear();
		spdlog::info("ResourceTracker: [RESET] Cleared {} tracked resource(s)", before);
	}

	static void InputThreadFunc()
	{
		auto& settings = Settings::Get();
		bool prevAdd = false;
		bool prevReset = false;

		while (g_running)
		{
			Sleep(80);

			HWND fg = GetForegroundWindow();
			if (!fg) {
				continue;
			}

			char title[256]{};
			GetWindowTextA(fg, title, sizeof(title));
			if (!strstr(title, "Starfield")) {
				continue;
			}

			bool curAdd = (GetAsyncKeyState(settings.addKey) & 0x8000) != 0;
			bool curReset = (GetAsyncKeyState(settings.resetKey) & 0x8000) != 0;

			if (curAdd && !prevAdd && g_gameReady) {
				OnAddKey();
			}

			if (curReset && !prevReset && g_gameReady) {
				OnResetKey();
			}

			prevAdd = curAdd;
			prevReset = curReset;
		}
	}

	void Init()
	{
		Settings::Get().Load();
		TrackedResources::Get().Load();

		auto& s = Settings::Get();
		spdlog::info("ResourceTracker: Initialized | Add key: {} ({}) | Reset key: {} ({})",
			VKToName(s.addKey), s.addKey, VKToName(s.resetKey), s.resetKey);
		spdlog::info("ResourceTracker: Loaded {} tracked resource(s) from file", TrackedResources::Get().Count());
		spdlog::info("ResourceTracker: Press [{}] at a workbench/research menu to toggle tracking", VKToName(s.addKey));
		spdlog::info("ResourceTracker: Press [{}] to reset local tracked list", VKToName(s.resetKey));

		g_running = true;
		g_inputThread = std::thread(InputThreadFunc);
	}

	void Shutdown()
	{
		g_running = false;
		if (g_inputThread.joinable()) {
			g_inputThread.join();
		}

		TrackedResources::Get().Save();
	}

	void SetGameReady(bool a_ready)
	{
		g_gameReady = a_ready;
	}
}
