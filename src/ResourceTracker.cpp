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

	static void ScanAndTrackMissingResources()
	{
		auto& tracked = TrackedResources::Get();
		std::size_t before = tracked.Count();

		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh)
		{
			spdlog::error("ResourceTracker: TESDataHandler not available");
			return;
		}

		std::vector<FormID> toAdd;

		// Scan BGSConstructibleObject (workbench recipes: weapon, armor, industrial, cooking, pharma)
		auto cobjIdx = static_cast<std::uint32_t>(RE::FormType::kCOBJ);
		auto& cobjArray = dh->formArrays[cobjIdx];
		for (auto& formPtr : cobjArray.formArray)
		{
			if (!formPtr)
				continue;

			auto* cobj = static_cast<RE::BGSConstructibleObject*>(formPtr.get());
			if (!cobj)
				continue;

			// Components come from BGSCraftableForm base class
			auto* craftable = static_cast<RE::BGSCraftableForm*>(cobj);
			if (!craftable->components)
				continue;

			for (auto& entry : *craftable->components)
			{
				auto* compForm = entry.first;
				if (compForm && compForm->GetFormID() != 0)
					toAdd.push_back(compForm->GetFormID());
			}
		}

		// Scan BGSResearchProjectForm (research lab recipes)
		auto rspjIdx = static_cast<std::uint32_t>(RE::FormType::kRSPJ);
		auto& rspjArray = dh->formArrays[rspjIdx];
		for (auto& formPtr : rspjArray.formArray)
		{
			if (!formPtr)
				continue;

			auto* craftable = static_cast<RE::BGSCraftableForm*>(formPtr.get());
			if (!craftable || !craftable->components)
				continue;

			for (auto& entry : *craftable->components)
			{
				auto* compForm = entry.first;
				if (compForm && compForm->GetFormID() != 0)
					toAdd.push_back(compForm->GetFormID());
			}
		}

		if (!toAdd.empty())
			tracked.AddBulk(toAdd);

		std::size_t after = tracked.Count();
		std::size_t added = (after > before) ? (after - before) : 0;
		spdlog::info("ResourceTracker: Scanned recipes. {} new resources tracked (total: {})", added, after);

		// Show in-game console notification
		auto* console = RE::ConsoleLog::GetSingleton();
		if (console)
		{
			console->Log("[ResourceTracker] Tracked {} new resource(s) (total: {})", added, after);
		}
	}

	static void OnAddKey()
	{
		ScanAndTrackMissingResources();
	}

	static void OnResetKey()
	{
		auto before = TrackedResources::Get().Count();
		TrackedResources::Get().Clear();
		spdlog::info("ResourceTracker: [RESET] Cleared {} tracked resource(s)", before);

		auto* console = RE::ConsoleLog::GetSingleton();
		if (console)
		{
			console->Log("[ResourceTracker] Cleared {} tracked resource(s)", before);
		}
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
		spdlog::info("ResourceTracker: Initialized  |  Add key: {} ({})  |  Reset key: {} ({})",
			VKToName(s.addKey), s.addKey, VKToName(s.resetKey), s.resetKey);
		spdlog::info("ResourceTracker: Loaded {} tracked resource(s) from file", TrackedResources::Get().Count());
		spdlog::info("ResourceTracker: Press [{}] at a workbench to track missing components", VKToName(s.addKey));
		spdlog::info("ResourceTracker: Press [{}] to reset the tracked list", VKToName(s.resetKey));

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
