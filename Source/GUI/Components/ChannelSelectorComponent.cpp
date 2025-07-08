#include "ChannelSelectorComponent.h"

ChannelSelectorComponent::ChannelSelectorComponent(juce::AudioDeviceManager& manager, AudioEngine& engine, ChannelType type, const juce::String& labelTextKey)
    : deviceManager(manager),
      audioEngine(engine),
      channelType(type),
      labelKey(labelTextKey)
{
    deviceManager.addChangeListener(this);
    LanguageManager::getInstance().addChangeListener(this);

    addAndMakeVisible(label);
    label.setText(LanguageManager::getInstance().get(labelKey), juce::dontSendNotification);

    addAndMakeVisible(selector);
    selector.setTextWhenNothingSelected(LanguageManager::getInstance().get("tracks.pleaseSelectDevice"));
    selector.onChange = [this] { handleSelectionChange(); };

    updateChannelList();
}

ChannelSelectorComponent::~ChannelSelectorComponent()
{
    deviceManager.removeChangeListener(this);
    LanguageManager::getInstance().removeChangeListener(this);
}

void ChannelSelectorComponent::resized()
{
    auto bounds = getLocalBounds();
    label.setBounds(bounds.removeFromLeft(80));
    bounds.removeFromLeft(5);
    selector.setBounds(bounds);
}

void ChannelSelectorComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &deviceManager)
    {
        updateChannelList();
    }
    else if (source == &LanguageManager::getInstance())
    {
        label.setText(LanguageManager::getInstance().get(labelKey), juce::dontSendNotification);
        selector.setTextWhenNothingSelected(LanguageManager::getInstance().get("tracks.pleaseSelectDevice"));
    }
}

void ChannelSelectorComponent::updateChannelList()
{
    // Tạm thời vô hiệu hóa callback để tránh bị gọi lại khi đang cập nhật
    juce::ScopedValueSetter<std::function<void()>> svs(selector.onChange, nullptr);

    switch (channelType)
    {
        case ChannelType::AudioInputMono:
            populateAudioInputChannels(false);
            break;
        case ChannelType::AudioInputStereo:
            populateAudioInputChannels(true);
            break;
        case ChannelType::AudioOutputStereo:
            populateAudioOutputChannels();
            break;
    }
}

void ChannelSelectorComponent::setSelectedChannelByName(const juce::String& channelName)
{
    for (int i = 0; i < selector.getNumItems(); ++i)
    {
        if (selector.getItemText(i) == channelName)
        {
            // Chọn đúng mục mà không kích hoạt callback
            selector.setSelectedId(selector.getItemId(i), juce::dontSendNotification);
            return;
        }
    }
}

void ChannelSelectorComponent::populateAudioInputChannels(bool isStereo)
{
    selector.clear(juce::dontSendNotification);
    availableChannelIndices.clear();

    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
    {
        auto activeChannels = deviceManager.getAudioDeviceSetup().inputChannels;
        auto channelNames = currentDevice->getInputChannelNames();
        int itemId = 1;

        if (isStereo)
        {
            for (int i = 0; i < channelNames.size() - 1; i += 2)
            {
                if (activeChannels[i] && activeChannels[i + 1])
                {
                    selector.addItem(channelNames[i] + " / " + channelNames[i + 1], itemId);
                    availableChannelIndices.add(i);
                    itemId++;
                }
            }
        }
        else // Mono
        {
            for (int i = 0; i < channelNames.size(); ++i)
            {
                if (activeChannels[i])
                {
                    selector.addItem(channelNames[i], itemId);
                    availableChannelIndices.add(i);
                    itemId++;
                }
            }
        }
    }
}

void ChannelSelectorComponent::populateAudioOutputChannels()
{
    selector.clear(juce::dontSendNotification);
    availableChannelIndices.clear();

    selector.addItem(LanguageManager::getInstance().get("menubar.noOutput"), -1);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        auto deviceSetup = deviceManager.getAudioDeviceSetup();
        auto channelNames = device->getOutputChannelNames();
        int itemId = 1;

        for (int i = 0; i < channelNames.size() - 1; i += 2)
        {
            if (deviceSetup.outputChannels[i] && deviceSetup.outputChannels[i + 1])
            {
                selector.addItem(channelNames[i] + " / " + channelNames[i + 1], itemId);
                availableChannelIndices.add(i); // Lưu chỉ số kênh trái
                itemId++;
            }
        }
    }
}


void ChannelSelectorComponent::handleSelectionChange()
{
    if (onSelectionChange == nullptr) return;

    const int selectedId = selector.getSelectedId();
    if (selectedId > 0 && (selectedId - 1) < availableChannelIndices.size())
    {
        // Lấy chỉ số kênh phần cứng thực sự từ map
        const int selectedChannelIndex = availableChannelIndices[selectedId - 1];
        onSelectionChange(selectedChannelIndex);
    }
    else if (selectedId == -1) // "No Output" được chọn
    {
        onSelectionChange(-1);
    }
}