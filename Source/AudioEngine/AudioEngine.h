#pragma once

#include "JuceHeader.h"
#include "TrackProcessor.h"
#include "MasterProcessor.h"
#include "AudioRecorder.h"
#include "../Data/PresetManager.h"
#include <functional>
#include "../GUI/Components/TrackPlayerComponent.h"

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

    std::function<void()> onDeviceStarted;

    // --- Các hàm điều khiển chung ---
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

    // --- Các hàm điều khiển cho REC & PLAY của kênh Master ---
    void startPlayback(const juce::File& file);
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    AudioRecorder& getAudioRecorder() { return *audioRecorder; }
    juce::AudioTransportSource& getTransportSource() { return playbackSource; }

    // --- Các hàm điều khiển cho Player của từng Track ---
    void startTrackPlayback(TrackPlayerComponent::PlayerType type, const juce::File& file);
    void stopTrackPlayback(TrackPlayerComponent::PlayerType type);
    void pauseTrackPlayback(TrackPlayerComponent::PlayerType type);
    void resumeTrackPlayback(TrackPlayerComponent::PlayerType type);
    void setTrackPlayerMute(TrackPlayerComponent::PlayerType type, bool shouldBeMuted);
    juce::AudioTransportSource& getTrackTransportSource(TrackPlayerComponent::PlayerType type);
    AudioRecorder& getTrackRecorder(TrackPlayerComponent::PlayerType type);

    // --- Các hàm điều khiển Project ---
    void startProjectRecording(const juce::String& projectName);
    void stopProjectRecording();
    bool isProjectRecording() const;
    void loadProject(const juce::File& projectJsonFile);
    void playLoadedProject();
    void stopLoadedProject();
    void seekProject(double newPositionRatio);
    bool isProjectPlaybackActive() const;
    juce::ValueTree& getProjectState();

    // --- Cấu trúc FX Chain ---
    struct FXChain
    {
        std::array<TrackProcessor, 4> processors;
        FXChain(const juce::Identifier& id1, const juce::Identifier& id2, const juce::Identifier& id3, const juce::Identifier& id4)
            : processors{ TrackProcessor(id1), TrackProcessor(id2), TrackProcessor(id3), TrackProcessor(id4) } {
        }
    };
    TrackProcessor* getFxProcessorForVocal(int index);
    TrackProcessor* getFxProcessorForMusic(int index);

private:
    juce::AudioDeviceManager& deviceManager;

    // Các bộ xử lý chính
    TrackProcessor vocalProcessor{ Identifiers::VocalProcessorState };
    TrackProcessor musicProcessor{ Identifiers::MusicProcessorState };
    MasterProcessor masterProcessor{ Identifiers::MasterProcessorState };

    // Các chuỗi FX
    FXChain vocalFxChain;
    FXChain musicFxChain;

    // Các bộ trộn
    std::unique_ptr<IdolAZ::SoundPlayer> soundPlayer;
    juce::MixerAudioSource soundboardMixer;
    juce::MixerAudioSource directOutputMixer;

    // Các buffer âm thanh
    juce::AudioBuffer<float> vocalBuffer, musicStereoBuffer, mixBuffer, soundboardBuffer, directOutputBuffer, fxSendBuffer;
    std::array<juce::AudioBuffer<float>, 4> vocalFxReturnBuffers;
    std::array<juce::AudioBuffer<float>, 4> musicFxReturnBuffers;
    juce::AudioBuffer<float> vocalPlayerBuffer, musicPlayerBuffer;

    // Các thành viên cho REC & PLAY kênh Master
    juce::AudioFormatManager formatManager;
    std::unique_ptr<AudioRecorder> audioRecorder;
    std::unique_ptr<juce::AudioFormatReaderSource> currentPlaybackReader;
    juce::AudioTransportSource playbackSource;

    // Các thành viên cho Player/Recorder của từng track
    std::unique_ptr<juce::AudioFormatReaderSource> vocalTrackReader, musicTrackReader;
    juce::AudioTransportSource vocalTrackSource, musicTrackSource;
    std::unique_ptr<AudioRecorder> vocalTrackRecorder, musicTrackRecorder;

    // Các thành viên của Project Record
    std::unique_ptr<AudioRecorder> rawVocalRecorder;
    std::unique_ptr<AudioRecorder> rawMusicRecorder;
    std::atomic<bool> isProjectPlaybackMode{ false }; // Đã sửa lỗi đánh máy "Moe"
    juce::String currentProjectName;
    juce::File currentVocalRawFile;
    juce::File currentMusicRawFile;
    juce::ValueTree projectState{ "ProjectState" };

    // Các biến trạng thái
    std::atomic<int> vocalInputChannel = -1, musicInputLeftChannel = -1, musicInputRightChannel = -1;
    std::atomic<int> selectedOutputLeftChannel = -1, selectedOutputRightChannel = -1;
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    TrackComponent* vocalTrackComponent = nullptr;
    TrackComponent* musicTrackComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};