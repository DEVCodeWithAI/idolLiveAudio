/*
  ==============================================================================

    AppState.h

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>

class MainComponent;

class AppState : public juce::ChangeBroadcaster
{
public:
    static constexpr int numQuickSlots = 5;

    static AppState& getInstance();

    void setPresetDirty(bool dirty);
    bool isPresetDirty() const;
    void setCurrentPresetName(const juce::String& name);
    juce::String getCurrentPresetName() const;
    void markAsSaved(const juce::String& newName);

    // Lock System
    void setSystemLocked(bool shouldBeLocked, const juce::String& password = {});
    bool unlockSystem(const juce::String& passwordAttempt);
    bool isSystemLocked() const;
    juce::String getPasswordHash() const;
    void loadLockState(bool isLocked, const juce::String& passwordHash);

    // Session State
    void saveState(MainComponent& mainComponent);
    bool loadAudioDeviceSetup(juce::AudioDeviceManager::AudioDeviceSetup& setupToFill, juce::String& loadedDeviceType);
    void loadPostDeviceState(MainComponent& mainComponent);
    juce::File getSessionFile() const;

    // Quick Presets
    void assignQuickPreset(int slotIndex, const juce::String& presetName);
    juce::String getQuickPresetName(int slotIndex) const;
    int getNumQuickPresetSlots() const;

    // Loading Flag
    void setIsLoadingPreset(bool isLoading);
    bool isLoadingPreset() const;

private:
    // This function is no longer used and has been removed.
    // void loadPresetAndWindows(MainComponent& mainComponent);

    AppState();
    ~AppState() override;

    bool dirty = false;
    juce::String currentPresetName;

    std::atomic<bool> systemLocked{ false };
    juce::String lockPasswordHash;

    std::atomic<bool> loadingPresetFlag{ false };

    juce::StringArray quickPresetSlots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};