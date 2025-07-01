#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Components/LevelMeter.h"
#include "../../AudioEngine/ProcessorBase.h"
#include "../Windows/FXChainWindow.h"

class AudioEngine;

class TrackComponent : public juce::Component,
    public juce::ChangeListener,
    public juce::ListBoxModel,
    public juce::AudioProcessorListener,
    public juce::ComboBox::Listener
{
public:
    enum class ChannelType { Vocal, Music };
    TrackComponent(const juce::String& trackNameKey, const juce::Colour& headerColour, ChannelType type);
    ~TrackComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    void setProcessor(ProcessorBase* proc);
    LevelMeter& getLevelMeter();
    void setAudioEngine(AudioEngine* engine);
    void populateInputChannels(const juce::StringArray& channelNames, const juce::Array<int>& channelIndices);

    void setSelectedInputChannelByName(const juce::String& channelName);

    void togglePluginBypass(int row);
    void deletePlugin(int row);
    void showPluginEditor(int row);

    void audioProcessorParameterChanged(juce::AudioProcessor* changedProcessor, int parameterIndex, float newValue) override;
    void audioProcessorChanged(juce::AudioProcessor* changedProcessor, const ChangeDetails& details) override;

    juce::ValueTree getOpenWindowsState() const;
    void restoreOpenWindows(const juce::ValueTree& state);

private:
    class PluginItemComponent;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

    void updateTexts();
    void updatePluginSelector(const juce::String& searchText = {});
    void addSelectedPlugin();
    void updateMuteButtonState();
    void addListenerToAllPlugins();
    void removeListenerFromAllPlugins();

    void openFxWindow(int index);

    ProcessorBase* processor = nullptr;
    AudioEngine* audioEngine = nullptr;
    ChannelType channelType;
    juce::Array<int> availableChannelIndices;

    juce::OwnedArray<juce::DocumentWindow> openPluginWindows;

    juce::String nameKey;
    juce::Colour colour;
    juce::Label trackLabel;
    juce::Label inputChannelLabel;
    juce::ComboBox inputChannelSelector;
    juce::TextButton lockButton;
    juce::TextButton muteButton;
    juce::Slider volumeSlider;
    LevelMeter levelMeter;

    juce::Label fxSectionLabel;
    std::array<juce::TextButton, 4> fxButtons;
    std::array<juce::TextButton, 4> fxMuteButtons;
    std::array<juce::Component::SafePointer<FXChainWindow>, 4> fxWindows;


    juce::Label pluginListLabel;
    juce::ListBox pluginListBox;
    juce::ComboBox addPluginSelector;
    juce::TextButton addButton;

    bool isUpdatingFromTextChange = false;
};