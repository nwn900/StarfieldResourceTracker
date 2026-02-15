#include "ResourceTracker.h"
#include "TrackedResources.h"
#include "DKUtil/Logger.hpp"

namespace ResourceTracker
{
	namespace
	{
		// Address Library IDs for Starfield (compatible with all versions via Address Library).
		// Obtain IDs from "Address Library for SFSE Plugins" (Nexus) offsets database.
		//
		// For magnifier icon: hook the code that draws a single item row in inventory/vendor/container UI,
		// then if TrackedResources::Get().Contains(itemFormID) draw the magnifier icon.
		//
		// For "Tag for search": when Research/Weapon/Armor/Industrial menu is open and user selects
		// "Tag missing", add shortfall component form IDs to TrackedResources (requires menu/form context).
	}

	void Init()
	{
		TrackedResources::Get().Load();

		// Allocate trampoline for future hooks (e.g. item list render detour).
		// SFSE::AllocTrampoline(1 << 10);

		// When Address Library IDs are available for item list rendering, add a REL hook here
		// to draw magnifier icon when the displayed item form ID is in TrackedResources::Get().
	}

	void Shutdown()
	{
		TrackedResources::Get().Save();
	}
}
