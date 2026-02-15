/*
 * Starfield Resource Tracker - SFSE Plugin
 *
 * Press [B] at a workbench to track missing crafting components.
 * Press [\] to reset the tracked list.
 * Keys are configurable in Data\SFSE\Plugins\ResourceTracker.ini.
 */

#include "ResourceTracker.h"

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
			ResourceTracker::SetGameReady(true);
			break;
		default:
			break;
		}
	}
}

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse);

	spdlog::info("ResourceTracker v1.0.0 loaded");

	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
