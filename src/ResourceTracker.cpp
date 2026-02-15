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

	static bool SimulateVirtualKeyPress(int vk)
	{
		if (vk <= 0 || vk > 0xFF) {
			return false;
		}

		INPUT inputs[2]{};
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = static_cast<WORD>(vk);
		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = static_cast<WORD>(vk);
		inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

		const UINT sent = SendInput(2, inputs, sizeof(INPUT));
		return sent == 2;
	}

	static void OnAddKey()
	{
		auto& settings = Settings::Get();
		if (!settings.useNativeTrackBridge) {
			spdlog::warn("ResourceTracker: B pressed, but native track bridge is disabled in INI");
			return;
		}

		if (SimulateVirtualKeyPress(settings.nativeTrackKey)) {
			spdlog::info("ResourceTracker: B -> forwarded native track key [{}]", VKToName(settings.nativeTrackKey));
		} else {
			spdlog::warn("ResourceTracker: failed to send native track key {}", settings.nativeTrackKey);
		}
	}

	static void OnResetKey()
	{
		auto before = TrackedResources::Get().Count();
		TrackedResources::Get().Clear();
		spdlog::info("ResourceTracker: [RESET] Cleared {} tracked resource(s)", before);
	}

	static void QueueAddAction()
	{
		if (auto* tasks = SFSE::GetTaskInterface(); tasks) {
			tasks->AddTask([]() {
				OnAddKey();
			});
		} else {
			spdlog::warn("ResourceTracker: Task interface unavailable for add action");
		}
	}

	static void QueueResetAction()
	{
		if (auto* tasks = SFSE::GetTaskInterface(); tasks) {
			tasks->AddTask([]() {
				OnResetKey();
			});
		} else {
			spdlog::warn("ResourceTracker: Task interface unavailable for reset action");
		}
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
				QueueAddAction();
			}

			if (curReset && !prevReset && g_gameReady) {
				QueueResetAction();
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
		spdlog::info("ResourceTracker: Native track bridge [{}], forwarded key: [{}]",
			s.useNativeTrackBridge ? "enabled" : "disabled",
			VKToName(s.nativeTrackKey));

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
