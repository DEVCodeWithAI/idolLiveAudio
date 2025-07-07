#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"

class AudioEngine;
class RecordingListWindow;

// <<< SỬA: Kế thừa thêm từ ValueTree::Listener >>>
class TrackPlayerComponent : public juce::Component,
    private juce::Timer,
    private juce::ChangeListener,
    private juce::ValueTree::Listener
{
public:
    enum class PlayerType { Vocal, Music };

    TrackPlayerComponent(PlayerType type, AudioEngine& engine);
    ~TrackPlayerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    // <<< THÊM: Hàm callback cho ValueTree::Listener >>>
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void updateTexts();
    juce::String formatTime(double seconds);
    void updatePlayButtonState();

    // <<< THÊM: Hàm helper để bật/tắt các nút >>>
    void setButtonsEnabled(bool shouldBeEnabled);

    void playTrack(const juce::File& fileToPlay);
    void stopTrack();
    void pauseTrack();
    void resumeTrack();
    void loadFile();
    void openTrackRecordingList();

    PlayerType playerType;
    AudioEngine& audioEngine;

    enum class State { Stopped, Playing, Paused };
    State playState{ State::Stopped };

    std::unique_ptr<RecordingListWindow> listWindow;

    juce::Label titleLabel;
    juce::TextButton recordButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton listButton;
    juce::TextButton loadButton;
    juce::ToggleButton muteButton;
    juce::Slider positionSlider;
    juce::Label currentTimeLabel;
    juce::Label totalTimeLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackPlayerComponent)
};