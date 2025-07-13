#pragma once

#include "JuceHeader.h"
#include "Data/PluginManager/PluginManager.h"
#include "Data/PresetManager.h"
// <<< ADDED: Include the new manager >>>
#include "GlobalHotkeyManager.h" 

// Forward declarations
class PresetManagerWindow;
class PluginManagerWindow;
class MainComponent;
class SplashWindow;

// --- FORWARD DECLARATION of our helper function ---
PluginManager& getSharedPluginManager();
PresetManager& getSharedPresetManager();

//==============================================================================
class idolLiveAudioApplication : public juce::JUCEApplication
{
public:
    idolLiveAudioApplication();

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void initialise(const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override {}
    void anotherInstanceStarted(const juce::String& commandLine) override { juce::ignoreUnused(commandLine); }

    PluginManager* getPluginManager() { return pluginManager.get(); }
    PresetManager* getPresetManager() { return presetManager.get(); }

    void showPresetManagerWindow();
    void showPluginManagerWindow();

    #if JUCE_WINDOWS
    GlobalHotkeyManager* getPublicGlobalHotkeyManager() { return globalHotkeyManager.get(); }
    #endif

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name);
        void closeButtonPressed() override;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

    std::unique_ptr<PluginManager> pluginManager;
    std::unique_ptr<PresetManager> presetManager;

    // <<< ADDED: The manager for global hotkeys >>>
#if JUCE_WINDOWS
    std::unique_ptr<GlobalHotkeyManager> globalHotkeyManager;
#endif

    std::unique_ptr<SplashWindow> splashWindow;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<PresetManagerWindow> presetManagerWindow;
    std::unique_ptr<PluginManagerWindow> pluginManagerWindow;
};