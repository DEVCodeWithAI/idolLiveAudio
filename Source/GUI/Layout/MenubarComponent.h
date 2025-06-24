#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"

// Forward declaration
class AudioEngine; // Thêm forward declaration cho AudioEngine

class MenubarComponent : public juce::Component,
    public juce::ChangeListener
{
public:
    MenubarComponent(juce::AudioDeviceManager& deviceManager, AudioEngine& audioEngine); // Thêm AudioEngine vào constructor
    ~MenubarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // <<< NEW METHOD >>>
    /** Sets the selected output channel pair in the ComboBox by its name. */
    void setSelectedOutputChannelPairByName(const juce::String& pairName);

private:
    void updateTexts();
    void updateButtonStates();
    void populateOutputChannels(); // Thêm hàm này để điền danh sách kênh đầu ra

    juce::AudioDeviceManager& deviceManager;
    AudioEngine& audioEngine; // Biến tham chiếu tới AudioEngine

    // UI Elements
    juce::ComboBox languageBox;
    juce::TextButton asioPanelButton, vmPanelButton;
    juce::TextButton audioSettingsButton;
    juce::ComboBox outputChannelSelector; // Thêm ComboBox cho kênh đầu ra
    juce::Label outputChannelLabel;      // Thêm Label cho ComboBox

    juce::Array<int> availableOutputStereoStartIndices; // Lưu chỉ số bắt đầu của các cặp kênh stereo đầu ra

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenubarComponent)
};