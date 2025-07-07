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

    // --- CÁC HÀM CHO TÍNH NĂNG KHÓA ---
    /** Locks the system with a given password. The password should be plain text. */
    void setSystemLocked(bool shouldBeLocked, const juce::String& password = {});

    /** Unlocks the system if the provided password is correct. */
    bool unlockSystem(const juce::String& passwordAttempt);

    /** Checks if the system is currently locked. */
    bool isSystemLocked() const;

    /** Returns the MD5 hash of the current lock password. */
    juce::String getPasswordHash() const;

    /** Loads lock state from a preset file. */
    void loadLockState(bool isLocked, const juce::String& passwordHash);


    // --- CÁC HÀM LƯU/TẢI SESSION ---
    void saveState(MainComponent& mainComponent);
    bool loadAudioDeviceSetup(juce::AudioDeviceManager::AudioDeviceSetup& setupToFill, juce::String& loadedDeviceType);
    void loadPostDeviceState(MainComponent& mainComponent);
    juce::File getSessionFile() const;

private:
    AppState();
    ~AppState() override;

    bool dirty = false;
    juce::String currentPresetName;

    // --- BIẾN THÀNH VIÊN CHO TÍNH NĂNG KHÓA ---
    std::atomic<bool> systemLocked{ false };
    juce::String lockPasswordHash;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};