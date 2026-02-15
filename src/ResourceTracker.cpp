#include "ResourceTracker.h"
#include "TrackedResources.h"
#include "Settings.h"

namespace ResourceTracker
{
	static std::atomic<bool> g_running{ false };
	static std::thread       g_inputThread;
	static std::atomic<bool> g_gameReady{ false };
	static bool              g_hintShown{ false };
	static bool              g_dumpedArrayHealth{ false };

	static std::string VKToName(int vk)
	{
		UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
		char name[64]{};
		if (GetKeyNameTextA(scanCode << 16, name, sizeof(name)) > 0)
			return name;
		return std::format("0x{:02X}", vk);
	}

	static std::size_t CollectComponentsFromArray(
		const RE::BSTArray<RE::BSTTuple3<RE::TESForm*, RE::BGSCurveForm*, RE::BGSTypedFormValuePair::SharedVal>>* a_components,
		std::vector<FormID>& a_out)
	{
		if (!a_components) {
			return 0;
		}

		std::size_t added = 0;
		for (const auto& entry : *a_components) {
			auto* compForm = entry.first;
			if (compForm && compForm->GetFormID() != 0) {
				a_out.push_back(compForm->GetFormID());
				++added;
			}
		}
		return added;
	}

	static void ScanAndTrackMissingResources()
	{
		if (!g_dumpedArrayHealth) {
			g_dumpedArrayHealth = true;
			std::size_t nonEmpty = 0;
			std::size_t largest = 0;
			std::uint32_t largestType = 0;
			const auto totalTypes = static_cast<std::uint32_t>(RE::FormType::kTotal);
			for (std::uint32_t i = 0; i < totalTypes; ++i) {
				const auto size = dh->formArrays[i].formArray.size();
				if (size > 0) {
					++nonEmpty;
				}
				if (size > largest) {
					largest = size;
					largestType = i;
				}
			}
			spdlog::info("ResourceTracker: DataHandler health - non-empty form arrays: {} / {}, largest array index: {}, size: {}",
				nonEmpty, totalTypes, largestType, largest);
		}

		auto& tracked = TrackedResources::Get();
		std::size_t before = tracked.Count();

		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh)
		{
			spdlog::error("ResourceTracker: TESDataHandler not available");
			return;
		}

		std::vector<FormID> toAdd;
		std::size_t cobjForms = 0;
		std::size_t cobjWithComponents = 0;
		std::size_t rspjForms = 0;
		std::size_t rspjWithComponents = 0;
		std::size_t rawComponents = 0;

		// Scan BGSConstructibleObject (workbench recipes: weapon, armor, industrial, cooking, pharma)
		auto cobjIdx = static_cast<std::uint32_t>(RE::FormType::kCOBJ);
		auto& cobjArray = dh->formArrays[cobjIdx];
		const RE::BSAutoReadLock cobjLocker(cobjArray.lock);
		for (auto& formPtr : cobjArray.formArray)
		{
			if (!formPtr)
				continue;

			++cobjForms;
			auto* cobj = formPtr->As<RE::BGSConstructibleObject>();
			if (!cobj) {
				continue;
			}

			// In Starfield, recipe ingredients are usually on BGSCraftableForm::components.
			// Some COBJ entries may also use the extra list at unk178.
			std::size_t localAdded = 0;
			localAdded += CollectComponentsFromArray(cobj->components, toAdd);
			localAdded += CollectComponentsFromArray(cobj->unk178, toAdd);
			rawComponents += localAdded;
			if (localAdded > 0) {
				++cobjWithComponents;
			}
		}

		// Scan BGSResearchProjectForm (research lab recipes)
		auto rspjIdx = static_cast<std::uint32_t>(RE::FormType::kRSPJ);
		auto& rspjArray = dh->formArrays[rspjIdx];
		const RE::BSAutoReadLock rspjLocker(rspjArray.lock);
		for (auto& formPtr : rspjArray.formArray)
		{
			if (!formPtr)
				continue;

			++rspjForms;
			auto* rspj = formPtr->As<RE::BGSResearchProjectForm>();
			if (!rspj) {
				continue;
			}

			auto localAdded = CollectComponentsFromArray(rspj->components, toAdd);
			rawComponents += localAdded;
			if (localAdded > 0) {
				++rspjWithComponents;
			}
		}

		if (!toAdd.empty())
			tracked.AddBulk(toAdd);

		std::size_t after = tracked.Count();
		std::size_t added = (after > before) ? (after - before) : 0;
		spdlog::info(
			"ResourceTracker: Scan stats - COBJ forms: {} (with components: {}), RSPJ forms: {} (with components: {}), raw components: {}",
			cobjForms, cobjWithComponents, rspjForms, rspjWithComponents, rawComponents);
		spdlog::info("ResourceTracker: Scanned recipes. {} new resources tracked (total: {})", added, after);
		RE::DebugNotification(std::format("ResourceTracker: {} new resource(s), total {}", added, after).c_str());

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
		RE::DebugNotification("[ResourceTracker] Tracked list reset");
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

			// Visible in the in-game console overlay, gives the player a persistent hint.
			if (!g_hintShown && g_gameReady) {
				auto* console = RE::ConsoleLog::GetSingleton();
				if (console) {
					console->SetUseConsoleOverlay(true);
					console->Log("[ResourceTracker] Press [%s] Add to the list  |  Press [%s] Reset list",
						VKToName(settings.addKey).c_str(),
						VKToName(settings.resetKey).c_str());
					RE::DebugNotification(std::format("ResourceTracker: [{}] Add to list, [{}] Reset list",
						VKToName(settings.addKey), VKToName(settings.resetKey)).c_str());
					g_hintShown = true;
				}
			}

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
