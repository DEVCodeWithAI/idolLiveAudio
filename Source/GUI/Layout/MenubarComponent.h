#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../GUI/Components/ChannelSelectorComponent.h" // <<< THÊM INCLUDE NÀY

class AudioEngine;
class LogoComponent;

class MenubarComponent : public juce::Component,
    public juce::ChangeListener
{
public:
    MenubarComponent(juce::AudioDeviceManager& deviceManager, AudioEngine& audioEngine);
    ~MenubarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // <<< THÊM GETTER NÀY >>>
    ChannelSelectorComponent* getOutputSelector() { return outputSelector.get(); }

private:
    void updateTexts();
    void updateButtonStates();
    // <<< XÓA: populateOutputChannels() và setSelectedOutputChannelPairByName() >>>

    juce::AudioDeviceManager& deviceManager;
    AudioEngine& audioEngine;

    std::unique_ptr<LogoComponent> logo;

    // UI Elements
    juce::ComboBox languageBox;
    juce::TextButton asioPanelButton, vmPanelButton;
    juce::TextButton audioSettingsButton;

    // <<< THAY THẾ CONTROL CŨ BẰNG COMPONENT MỚI >>>
    std::unique_ptr<ChannelSelectorComponent> outputSelector;

    // <<< XÓA: availableOutputStereoStartIndices >>>

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenubarComponent)
};