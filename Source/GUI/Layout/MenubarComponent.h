#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"

// Forward declaration
class AudioEngine;
// <<< THÊM: Khai báo chuyển tiếp cho component logo >>>
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

    void setSelectedOutputChannelPairByName(const juce::String& pairName);

private:
    void updateTexts();
    void updateButtonStates();
    void populateOutputChannels();

    juce::AudioDeviceManager& deviceManager;
    AudioEngine& audioEngine;

    // <<< THÊM: Component cho Logo >>>
    std::unique_ptr<LogoComponent> logo;

    // UI Elements
    juce::ComboBox languageBox;
    juce::TextButton asioPanelButton, vmPanelButton;
    juce::TextButton audioSettingsButton;
    juce::ComboBox outputChannelSelector;
    juce::Label outputChannelLabel;

    juce::Array<int> availableOutputStereoStartIndices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenubarComponent)
};