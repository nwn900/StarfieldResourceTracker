#pragma once

namespace ResourceTracker
{
	// Initialize hooks and menu integration. Called from SFSEPlugin_Load / PostLoad.
	void Init();

	// Shutdown (optional).
	void Shutdown();
}
