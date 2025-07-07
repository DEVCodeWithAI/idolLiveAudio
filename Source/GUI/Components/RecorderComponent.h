#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/AudioEngine.h"

// Forward declaration
class RecordingListWindow;

class RecorderComponent : public juce::Component, public juce::Timer, private juce::ChangeListener
{
public:
    RecorderComponent(AudioEngine& engine);
    ~RecorderComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updateButtonStates();
    void updateTexts();
    juce::String formatTime(double seconds);
    void openRecordingList();
    void startRecording();
    void stopRecording();
    void startPlayback(const juce::File& fileToPlay);
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();


    AudioEngine& audioEngine;

    std::unique_ptr<RecordingListWindow> listWindow;

    juce::Label titleLabel;
    
    juce::TextButton recordButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton pauseButton;
    juce::TextButton recordedListButton;
    juce::Slider positionSlider;
    juce::Label currentTimeLabel;
    juce::Label totalTimeLabel;

    std::unique_ptr<RecordingListWindow> recordingListWindow;

    enum class PlayState
    {
        Stopped,
        Playing,
        Paused,
        Recording
    };

    PlayState state { PlayState::Stopped };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecorderComponent)
};