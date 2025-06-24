#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/AudioEngine.h"
#include <functional>

class PresetBarComponent : public juce::Component, public juce::ChangeListener
{
public:
    PresetBarComponent(AudioEngine& engine);
    ~PresetBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    /**
     * @brief Prompts the user to save the current configuration as a new preset file.
     * @param onComplete A callback function that receives 'true' if the file was saved,
     * 'false' if the user cancelled.
    */
    void saveAsNewPreset(std::function<void(bool)> onComplete);

    /**
     * @brief Programmatically loads a preset by its name, bypassing any user dialogs.
     * Used for restoring session state.
     * @param name The file name of the preset without the .xml extension.
    */
    void loadPresetByName(const juce::String& name);

private:
    void updateTexts();
    void populatePresetBox();
    void savePreset();
    void loadPresetFromButton();
    void performLoadTask();

    AudioEngine& audioEngine;

    juce::Label presetRunningLabel, presetRunningValue, quickChoiceLabel;
    juce::ComboBox quickChoiceBox;
    juce::TextButton loadButton;
    juce::TextButton savePresetButton, managePresetsButton;
};