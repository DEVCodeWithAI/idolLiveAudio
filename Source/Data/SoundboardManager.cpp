#include "Application/Application.h"
#include "Data/SoundboardManager.h"

// --- DEFINITION of our helper function ---
SoundboardProfileManager& getSharedSoundboardProfileManager()
{
	// This is a placeholder for where you will create and manage the single instance
	// of SoundboardProfileManager. For now, we'll use a static instance.
	// A better approach would be to have this owned by your main application class,
	// similar to PluginManager.
	static SoundboardProfileManager instance;
	return instance;
}