#pragma once

#include <JuceHeader.h>

class MainComponent;

class AppState : public juce::ChangeBroadcaster
{
public:
    static AppState& getInstance();

    void setPresetDirty(bool dirty);
    bool isPresetDirty() const;
    void setCurrentPresetName(const juce::String& name);
    juce::String getCurrentPresetName() const;
    void markAsSaved(const juce::String& newName);

    void saveState(MainComponent& mainComponent);

    bool loadAudioDeviceSetup(juce::AudioDeviceManager::AudioDeviceSetup& setupToFill, juce::String& loadedDeviceType);

    void loadPostDeviceState(MainComponent& mainComponent);

    // <<< FIX: Moved getSessionFile() to public section >>>
    juce::File getSessionFile() const;

private:
    AppState();
    ~AppState() override;

    bool dirty = false;
    juce::String currentPresetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};