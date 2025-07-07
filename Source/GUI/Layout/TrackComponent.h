#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Components/LevelMeter.h"
#include "../../AudioEngine/ProcessorBase.h"
#include "../Windows/FXChainWindow.h"
#include "../../GUI/Components/TrackPlayerComponent.h"
#include "../../GUI/Components/FxSendsComponent.h"

class AudioEngine; // Forward declaration

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
    void audioProcessorParameterChanged(juce::AudioProcessor* changedProcessor, int parameterIndex, float newValue) override;
    void audioProcessorChanged(juce::AudioProcessor* changedProcessor, const ChangeDetails& details) override;

    // --- Getters và Setters ---
    void setProcessor(ProcessorBase* proc);
    LevelMeter& getLevelMeter();
    void setAudioEngine(AudioEngine* engine);
    void populateInputChannels(const juce::StringArray& channelNames, const juce::Array<int>& channelIndices);
    void setSelectedInputChannelByName(const juce::String& channelName);

    // --- Quản lý Plugin UI ---
    void togglePluginBypass(int row);
    void deletePlugin(int row);
    void showPluginEditor(int row);
    juce::ValueTree getOpenWindowsState() const;
    void restoreOpenWindows(const juce::ValueTree& state);
    void closeAllPluginWindows();

    // --- Hàm public để FxSendsComponent có thể gọi ---
    void openFxWindow(int index);
    void toggleFxMute(int index, bool shouldBeMuted);

private:
    // --- Các lớp con và khai báo private ---
    friend class FxSendsComponent; // Cho phép FxSendsComponent truy cập thành viên private nếu cần
    class PluginItemComponent;

    // --- Triển khai ListBoxModel ---
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

    // --- Các hàm trợ giúp private ---
    void updateTexts();
    void updatePluginSelector(const juce::String& searchText = {});
    void addSelectedPlugin();
    void updateMuteButtonState();
    void addListenerToAllPlugins();
    void removeListenerFromAllPlugins();

    // --- Các biến thành viên ---
    ProcessorBase* processor = nullptr;
    AudioEngine* audioEngine = nullptr;
    ChannelType channelType;
    juce::Array<int> availableChannelIndices;

    juce::OwnedArray<juce::DocumentWindow> openPluginWindows;
    std::array<juce::Component::SafePointer<FXChainWindow>, 4> fxWindows;

    // --- Các UI Component ---
    juce::String nameKey;
    juce::Colour colour;
    juce::Label trackLabel;
    juce::Label inputChannelLabel;
    juce::ComboBox inputChannelSelector;
    juce::TextButton lockButton;
    juce::TextButton muteButton;
    juce::Slider volumeSlider;
    LevelMeter levelMeter;

    std::unique_ptr<TrackPlayerComponent> trackPlayer;
    std::unique_ptr<FxSendsComponent> fxSends;

    juce::Label pluginListLabel;
    juce::ListBox pluginListBox;
    juce::ComboBox addPluginSelector;
    juce::TextButton addButton;

    bool isUpdatingFromTextChange = false;
};