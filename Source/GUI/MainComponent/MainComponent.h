#pragma once

#include "JuceHeader.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Components/CustomLookAndFeel.h"
#include "../../Data/CommandIDs.h"
#include <functional>

// Forward declarations
class MenubarComponent;
class PresetBarComponent;
class TrackComponent;
class MasterUtilityComponent;
class PluginManagementComponent;
class StatusBarComponent;
class ProjectManagerComponent;
class BeatManagerComponent;

class MainComponent : public juce::Component,
    public juce::ChangeListener,
    public juce::ApplicationCommandTarget,
    public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // juce::ApplicationCommandTarget overrides
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;
    void timerCallback() override;

    /**
     * @brief Checks for unsaved changes and prompts the user if necessary.
     * @param callback A function that will be called with 'true' if the application
     * is clear to close, or 'false' if the user cancelled the action.
    */
    void attemptToClose(std::function<void(bool)> callback);

    // --- Getters for AppState to access core components ---
    juce::AudioDeviceManager& getAudioDeviceManager() { return *deviceManager; }
    AudioEngine& getAudioEngine() { return audioEngine; }
    TrackComponent& getVocalTrack() { return *vocalTrack; }
    TrackComponent& getMusicTrack() { return *musicTrack; }
    MasterUtilityComponent& getMasterUtilityComponent() { return *masterUtilityColumn; }
    PresetBarComponent& getPresetBar() { return *presetBar; }

    // <<< NEW GETTER >>>
    MenubarComponent* getMenubarComponent() { return menubar.get(); }


private:
    class GlassPane;

    void reloadHotkeysFromSlots();

    void setBeatManagerExpanded(bool shouldBeExpanded);

    juce::ComponentAnimator animator;
    bool isBeatManagerExpanded = false;

    std::unique_ptr<GlassPane> glassPane;

    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    AudioEngine audioEngine;
    CustomLookAndFeel customLookAndFeel;

    juce::ApplicationCommandManager commandManager;
    std::unique_ptr<ProjectManagerComponent> projectManager;

    std::unique_ptr<MenubarComponent> menubar;
    std::unique_ptr<PresetBarComponent> presetBar;
    std::unique_ptr<TrackComponent> vocalTrack;
    std::unique_ptr<TrackComponent> musicTrack;
    std::unique_ptr<BeatManagerComponent> beatManager;
    std::unique_ptr<MasterUtilityComponent> masterUtilityColumn;
    std::unique_ptr<PluginManagementComponent> pluginManagement;
    std::unique_ptr<StatusBarComponent> statusBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};