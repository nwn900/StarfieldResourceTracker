#include "ResourceTracker.h"
#include "Settings.h"
#include "TrackedResources.h"

namespace ResourceTracker
{
	static std::atomic<bool> g_running{ false };
	static std::thread       g_inputThread;
	static std::atomic<bool> g_gameReady{ false };
	static std::atomic<bool> g_researchMenuOpen{ false };
	static std::atomic<bool> g_craftingMenuOpen{ false };
	static std::atomic<bool> g_menuSinkRegistered{ false };

	namespace
	{
		class MenuOpenCloseSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(
				const RE::MenuOpenCloseEvent& a_event,
				RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				const std::string name = a_event.menuName.c_str();
				const bool open = a_event.opening;

				// Diagnostic log to discover exact menu names on this runtime.
				spdlog::info("ResourceTracker: Menu {}: {}", open ? "opened" : "closed", name);

				const bool isResearch = name.find("Research") != std::string::npos;
				const bool isCrafting =
					name.find("Craft") != std::string::npos ||
					name.find("Workshop") != std::string::npos ||
					name.find("Workbench") != std::string::npos;

				if (isResearch) {
					g_researchMenuOpen = open;
				}
				if (isCrafting) {
					g_craftingMenuOpen = open;
				}

				return RE::BSEventNotifyControl::kContinue;
			}
		};

		MenuOpenCloseSink g_menuSink;
	}

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
		if (g_researchMenuOpen) {
			if (DispatchUIEvent<RE::ResearchMenu_ToggleTrackingProject>()) {
				spdlog::info("ResourceTracker: B -> ResearchMenu_ToggleTrackingProject");
				return;
			}
		}

		if (g_craftingMenuOpen) {
			if (DispatchUIEvent<RE::CraftingMenu_ToggleTracking>()) {
				spdlog::info("ResourceTracker: B -> CraftingMenu_ToggleTracking");
				return;
			}
		}

		spdlog::info("ResourceTracker: B pressed, but no supported menu context detected");
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

		if (auto* ui = RE::UI::GetSingleton(); ui) {
			ui->RegisterSink<RE::MenuOpenCloseEvent>(&g_menuSink);
			g_menuSinkRegistered = true;
			spdlog::info("ResourceTracker: Registered menu open/close sink");
		} else {
			spdlog::warn("ResourceTracker: UI singleton unavailable; menu context detection disabled");
		}

		g_running = true;
		g_inputThread = std::thread(InputThreadFunc);
	}

	void Shutdown()
	{
		g_running = false;
		if (g_inputThread.joinable()) {
			g_inputThread.join();
		}

		if (g_menuSinkRegistered) {
			if (auto* ui = RE::UI::GetSingleton(); ui) {
				ui->UnregisterSink<RE::MenuOpenCloseEvent>(&g_menuSink);
			}
			g_menuSinkRegistered = false;
		}

		TrackedResources::Get().Save();
	}

	void SetGameReady(bool a_ready)
	{
		g_gameReady = a_ready;
	}
}
