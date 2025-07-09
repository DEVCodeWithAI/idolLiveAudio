#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Data/AppState.h" 
#include <functional>
#include <array>

// <<< ADDED: Forward declaration for the new SlotComponent >>>
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

    // <<< MOVED: These methods need to be public to be called by PresetSlotComponent >>>
    void loadQuickPreset(int slotIndex);
    void showAssignMenuForSlot(int slotIndex);

private:
    void updateTexts();
    void updateQuickButtonLabels();
    void handleSaveAction();
    void performLoadTask(const juce::String& presetName);

    AudioEngine& audioEngine;

    juce::Label presetRunningLabel, presetRunningValue, quickChoiceLabel;

    std::array<std::unique_ptr<PresetSlotComponent>, AppState::numQuickSlots> quickLoadSlots;

    juce::TextButton savePresetButton, managePresetsButton;
};