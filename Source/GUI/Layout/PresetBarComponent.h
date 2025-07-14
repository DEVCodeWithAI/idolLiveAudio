/*
  ==============================================================================

    PresetBarComponent.h

  ==============================================================================
*/
#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Data/AppState.h" 
#include <functional>
#include <array>

class PresetSlotComponent;

class PresetBarComponent : public juce::Component,
    public juce::ChangeListener
{
public:
    PresetBarComponent(AudioEngine& engine);
    ~PresetBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void saveAsNewPreset(std::function<void(bool)> onComplete);
    void loadPresetByName(const juce::String& name);

    void loadQuickPreset(int slotIndex);
    void showAssignMenuForSlot(int slotIndex);

private:
    void updateTexts();
    void updateQuickButtonLabels();
    void handleSaveAction();

    void loadPresetTask(const juce::String& presetName);

    // <<< FIXED: Updated function signature to match implementation >>>
    void performFullReload(const juce::ValueTree& newState, const juce::String& presetName, bool isLocked, const juce::String& passwordHash);

    AudioEngine& audioEngine;

    juce::Label presetRunningLabel, presetRunningValue, quickChoiceLabel;
    std::array<std::unique_ptr<PresetSlotComponent>, AppState::numQuickSlots> quickLoadSlots;
    juce::TextButton savePresetButton, managePresetsButton;
};