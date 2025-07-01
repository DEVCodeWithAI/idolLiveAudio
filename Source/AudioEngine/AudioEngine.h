/*
  ==============================================================================

    AudioEngine.h
    (Error Fix)

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"
#include "TrackProcessor.h"
#include "MasterProcessor.h"
#include "../Data/PresetManager.h" // <<< MODIFIED: Included PresetManager.h for Identifiers
#include <functional>

namespace IdolAZ { class SoundPlayer; }

class TrackComponent;
class MasterUtilityComponent;

namespace juce { class AudioDeviceManager; }

// <<< MODIFIED: The Identifiers namespace has been moved to PresetManager.h >>>


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

    std::function<void()> onDeviceStarted;

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

    struct FXChain
    {
        std::array<TrackProcessor, 4> processors;

        FXChain(const juce::Identifier& id1, const juce::Identifier& id2, const juce::Identifier& id3, const juce::Identifier& id4)
            : processors{ TrackProcessor(id1), TrackProcessor(id2), TrackProcessor(id3), TrackProcessor(id4) }
        {
        }
    };

    TrackProcessor* getFxProcessorForVocal(int index);
    TrackProcessor* getFxProcessorForMusic(int index);

private:
    juce::AudioDeviceManager& deviceManager;

    // Main Processors
    TrackProcessor vocalProcessor{ Identifiers::VocalProcessorState };
    TrackProcessor musicProcessor{ Identifiers::MusicProcessorState };
    MasterProcessor masterProcessor{ Identifiers::MasterProcessorState };

    FXChain vocalFxChain;
    FXChain musicFxChain;

    std::unique_ptr<IdolAZ::SoundPlayer> soundPlayer;

    // Main buffers
    juce::AudioBuffer<float> vocalBuffer, musicStereoBuffer, mixBuffer, soundboardBuffer;
    juce::AudioBuffer<float> fxSendBuffer;
    std::array<juce::AudioBuffer<float>, 4> vocalFxReturnBuffers;
    std::array<juce::AudioBuffer<float>, 4> musicFxReturnBuffers;


    std::atomic<int> vocalInputChannel = -1, musicInputLeftChannel = -1, musicInputRightChannel = -1;
    std::atomic<int> selectedOutputLeftChannel = -1, selectedOutputRightChannel = -1;
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    TrackComponent* vocalTrackComponent = nullptr;
    TrackComponent* musicTrackComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};