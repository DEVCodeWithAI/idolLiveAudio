#pragma once

#include <JuceHeader.h>

class MainComponent;

class AppState : public juce::ChangeBroadcaster
{
public:
    // <<< ADDED: A constant for the number of slots >>>
    static constexpr int numQuickSlots = 5;

    static AppState& getInstance();

    void setPresetDirty(bool dirty);
    bool isPresetDirty() const;
    void setCurrentPresetName(const juce::String& name);
    juce::String getCurrentPresetName() const;
    void markAsSaved(const juce::String& newName);

    // --- CÁC HÀM CHO TÍNH NĂNG KHÓA ---
    void setSystemLocked(bool shouldBeLocked, const juce::String& password = {});
    bool unlockSystem(const juce::String& passwordAttempt);
    bool isSystemLocked() const;
    juce::String getPasswordHash() const;
    void loadLockState(bool isLocked, const juce::String& passwordHash);


    // --- CÁC HÀM LƯU/TẢI SESSION ---
    void saveState(MainComponent& mainComponent);
    bool loadAudioDeviceSetup(juce::AudioDeviceManager::AudioDeviceSetup& setupToFill, juce::String& loadedDeviceType);
    void loadPostDeviceState(MainComponent& mainComponent);
    juce::File getSessionFile() const;

    // <<< MODIFIED: Updated comments and consistency >>>
    /** Assigns a preset name to a quick load slot. */
    void assignQuickPreset(int slotIndex, const juce::String& presetName);
    /** Gets the name of the preset assigned to a slot. */
    juce::String getQuickPresetName(int slotIndex) const;
    /** Gets the number of available quick slots. */
    int getNumQuickPresetSlots() const;

private:
    void loadPresetAndWindows(MainComponent& mainComponent);

    AppState();
    ~AppState() override;

    bool dirty = false;
    juce::String currentPresetName;

    std::atomic<bool> systemLocked{ false };
    juce::String lockPasswordHash;

    juce::StringArray quickPresetSlots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};