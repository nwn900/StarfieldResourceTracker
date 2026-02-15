/*
 * Starfield Resource Tracker - SFSE Plugin
 * Lets the player tag missing resources for research, weapon/armor mods, and industrial fabrication,
 * and shows a magnifier icon when those resources appear in vendor menus, containers, or when selected.
 * Uses Address Library for SFSE Plugins for version-independent compatibility.
 */

#include "DKUtil/Hook.hpp"
#include "ResourceTracker.h"
#include "TrackedResources.h"

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type)
		{
		case SFSE::MessagingInterface::kPostLoad:
			ResourceTracker::Init();
			break;
		case SFSE::MessagingInterface::kPostPostLoad:
			break;
		default:
			break;
		}
	}
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	MessageBoxA(NULL, "Resource Tracker loaded. Attach debugger if needed.", Plugin::NAME.data(), NULL);
#endif

	SFSE::Init(a_sfse, false);
	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded (Address Library enabled for all game versions)", Plugin::NAME, Plugin::Version);

	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
