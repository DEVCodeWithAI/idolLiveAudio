/*
  ==============================================================================

    AudioEngine.h
    (Final fix using std::function callback)

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "TrackProcessor.h"
#include "MasterProcessor.h"
#include "../Data/PresetManager.h"
#include <functional> // Include for std::function

namespace IdolAZ { class SoundPlayer; }

class TrackComponent;
class MasterUtilityComponent;

namespace juce { class AudioDeviceManager; }

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine(juce::AudioDeviceManager& manager);
    ~AudioEngine() override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
        float* const* outputChannelData, int numOutputChannels,
        int numSamples, const juce::AudioIODeviceCallbackContext& context) override;

    // Callback to be set by MainComponent, triggered when the device is ready.
    std::function<void()> onDeviceStarted;

    // Other methods...
    void setVocalInputChannel(int channelIndex);
    void setMusicInputChannel(int leftChannelIndex);
    void setSelectedOutputChannels(int leftChannelIndex, int rightChannelIndex);
    juce::String getVocalInputChannelName() const;
    juce::String getMusicInputChannelName() const;
    juce::String getSelectedOutputChannelPairName() const;
    void setVocalInputChannelByName(const juce::String& channelName);
    void setMusicInputChannelByName(const juce::String& stereoPairName);
    void setSelectedOutputChannelsByName(const juce::String& stereoPairName);
    int getSelectedOutputLeftChannel() const;
    int getSelectedOutputRightChannel() const;
    void linkTrackComponents(TrackComponent& vocalTrackComp, TrackComponent& musicTrackComp);
    void linkMasterComponents(MasterUtilityComponent& comp);
    TrackProcessor& getVocalProcessor();
    TrackProcessor& getMusicProcessor();
    MasterProcessor& getMasterProcessor();
    IdolAZ::SoundPlayer& getSoundPlayer();
    void updateActiveInputChannels(juce::AudioDeviceManager& manager);

private:
    juce::AudioDeviceManager& deviceManager;

    TrackProcessor vocalProcessor{ Identifiers::VocalProcessorState };
    TrackProcessor musicProcessor{ Identifiers::MusicProcessorState };
    MasterProcessor masterProcessor{ Identifiers::MasterProcessorState };
    std::unique_ptr<IdolAZ::SoundPlayer> soundPlayer;
    juce::AudioBuffer<float> vocalBuffer, musicStereoBuffer, mixBuffer, soundboardBuffer;
    std::atomic<int> vocalInputChannel = -1, musicInputLeftChannel = -1, musicInputRightChannel = -1;
    std::atomic<int> selectedOutputLeftChannel = -1, selectedOutputRightChannel = -1;
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    TrackComponent* vocalTrackComponent = nullptr;
    TrackComponent* musicTrackComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};