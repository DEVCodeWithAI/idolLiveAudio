#pragma once

#include <JuceHeader.h>
#include <memory>
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Components/LevelMeter.h"
#include "../../AudioEngine/MasterProcessor.h"
#include "SoundboardComponent.h"
#include "../../AudioEngine/AudioEngine.h"

// <<< THÊM INCLUDE NÀY >>>
#include "../../GUI/Components/RecorderComponent.h"

class MasterPluginsWindow;

class MasterUtilityComponent : public juce::Component,
    public juce::ChangeListener
{
public:
    MasterUtilityComponent(AudioEngine& engine);
    ~MasterUtilityComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setMasterProcessor(MasterProcessor* proc);
    LevelMeter& getMasterLevelMeter();

    // --- NEW: Methods for session state ---
    /** Gets the state of all open plugin windows for the master track. */
    juce::ValueTree getOpenWindowsState() const;

    /** Restores the open plugin windows for the master track from a saved state. */
    void restoreOpenWindows(const juce::ValueTree& state);
    void closeAllPluginWindows();


private:
    void updateTexts();
    void updateMuteButtonAppearance();
    void updateLockState();

    AudioEngine& audioEngine;

    // Master Processor
    MasterProcessor* masterProcessor = nullptr;

    std::unique_ptr<MasterPluginsWindow> masterPluginsWindow;

    // <<< THÊM COMPONENT MỚI >>>
    std::unique_ptr<RecorderComponent> recorderComponent;

    // Section 1: Soundboard
    std::unique_ptr<SoundboardComponent> soundboardComponent;

    // Section 2: Buttons
    juce::TextButton masterPluginsButton;
    juce::TextButton quickKeySettingsButton;

    // Section 3: Master Controls
    juce::Label masterVolumeLabel;
    juce::Slider masterVolumeSlider;
    juce::TextButton masterMuteButton;
    LevelMeter masterLevelMeter;
};