#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/AudioEngine.h"

class ChannelSelectorComponent : public juce::Component,
    private juce::ChangeListener
{
public:
    enum class ChannelType
    {
        AudioInputMono,
        AudioInputStereo,
        AudioOutputStereo
    };

    // <<< SỬA LỖI 1: Khai báo callback bằng std::function >>>
    // Đây là cách làm hiện đại, an toàn và linh hoạt trong C++.
    using OnChangeCallback = std::function<void(int newChannelIndex)>;

    ChannelSelectorComponent(juce::AudioDeviceManager& manager, AudioEngine& engine, ChannelType type, const juce::String& labelTextKey);
    ~ChannelSelectorComponent() override;

    void resized() override;

    void updateChannelList();

    // <<< SỬA LỖI 2: Thêm khoảng trắng cho tên hàm >>>
    void setSelectedChannelByName(const juce::String& channelName);

    // <<< SỬA LỖI 3: Khai báo đúng biến thành viên cho callback >>>
    OnChangeCallback onSelectionChange = nullptr;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void populateAudioInputChannels(bool isStereo);
    void populateAudioOutputChannels();
    void handleSelectionChange();

    juce::AudioDeviceManager& deviceManager;
    AudioEngine& audioEngine;
    const ChannelType channelType;
    const juce::String labelKey;

    juce::Label label;
    juce::ComboBox selector;

    juce::Array<int> availableChannelIndices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelectorComponent)
};