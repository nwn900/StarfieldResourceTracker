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

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	MessageBoxA(NULL, "Resource Tracker loaded. Attach debugger if needed.", Plugin::NAME.data(), NULL);
#endif

	SFSE::Init(a_sfse, false);
	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
