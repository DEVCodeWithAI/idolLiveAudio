/*
  ==============================================================================

    AudioEngine.cpp
    (Final fix using std::function callback)

  ==============================================================================
*/

#include "AudioEngine.h"
#include "../GUI/Layout/TrackComponent.h"
#include "../GUI/Layout/MasterUtilityComponent.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "SoundPlayer.h"

AudioEngine::AudioEngine(juce::AudioDeviceManager& manager)
    : deviceManager(manager)
{
    soundPlayer = std::make_unique<IdolAZ::SoundPlayer>();
}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    DBG("audioDeviceAboutToStart CALLED! Device Name: " << device->getName()
        << ", Sample Rate: " << device->getCurrentSampleRate()
        << ", Buffer Size: " << device->getCurrentBufferSizeSamples());
    currentSampleRate = device->getCurrentSampleRate();
    currentBlockSize = device->getCurrentBufferSizeSamples();

    juce::dsp::ProcessSpec stereoSpec{ currentSampleRate, static_cast<uint32_t>(currentBlockSize), 2 };

    vocalProcessor.prepare(stereoSpec);
    musicProcessor.prepare(stereoSpec);
    masterProcessor.prepare(stereoSpec);

    soundPlayer->prepareToPlay(currentBlockSize, currentSampleRate);

    vocalBuffer.setSize(2, currentBlockSize);
    musicStereoBuffer.setSize(2, currentBlockSize);
    mixBuffer.setSize(2, currentBlockSize);
    soundboardBuffer.setSize(2, currentBlockSize);

    vocalProcessor.reset();
    musicProcessor.reset();
    masterProcessor.reset();

    // --- NEW LOGIC: Trigger the callback ---
    if (onDeviceStarted)
    {
        // Use callAsync to run the callback on the main message thread,
        // which is safe for UI operations and completing the session restore.
        juce::MessageManager::callAsync(onDeviceStarted);
    }
}

void AudioEngine::audioDeviceStopped()
{
    DBG("Audio device stopped."); 
    vocalProcessor.reset();
    musicProcessor.reset();
    masterProcessor.reset();
    soundPlayer->releaseResources();

    currentSampleRate = 0.0;
    currentBlockSize = 0;
    vocalInputChannel.store(-1);
    musicInputLeftChannel.store(-1);
    musicInputRightChannel.store(-1);
    selectedOutputLeftChannel.store(-1);
    selectedOutputRightChannel.store(-1);

    if (vocalTrackComponent != nullptr)
        vocalTrackComponent->populateInputChannels({}, {});
    if (musicTrackComponent != nullptr)
        musicTrackComponent->populateInputChannels({}, {});
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);
    juce::ScopedNoDenormals noDenormals;

    const int currentVocalIn = vocalInputChannel.load();
    const int currentMusicLeftIn = musicInputLeftChannel.load();
    const int currentMusicRightIn = musicInputRightChannel.load();
    const int currentOutputLeft = selectedOutputLeftChannel.load();
    const int currentOutputRight = selectedOutputRightChannel.load();

    vocalBuffer.clear();
    musicStereoBuffer.clear();
    mixBuffer.clear();
    soundboardBuffer.clear();

    if (juce::isPositiveAndBelow(currentVocalIn, numInputChannels))
    {
        const float* monoInputSignal = inputChannelData[currentVocalIn];
        vocalBuffer.copyFrom(0, 0, monoInputSignal, numSamples);
        vocalBuffer.copyFrom(1, 0, monoInputSignal, numSamples);
        vocalProcessor.process(vocalBuffer);
    }

    if (juce::isPositiveAndBelow(currentMusicLeftIn, numInputChannels) &&
        juce::isPositiveAndBelow(currentMusicRightIn, numInputChannels))
    {
        musicStereoBuffer.copyFrom(0, 0, inputChannelData[currentMusicLeftIn], numSamples);
        musicStereoBuffer.copyFrom(1, 0, inputChannelData[currentMusicRightIn], numSamples);
        musicProcessor.process(musicStereoBuffer);
    }

    juce::AudioSourceChannelInfo soundboardChannelInfo(&soundboardBuffer, 0, numSamples);
    soundPlayer->getNextAudioBlock(soundboardChannelInfo);

    mixBuffer.addFrom(0, 0, vocalBuffer, 0, 0, numSamples);
    mixBuffer.addFrom(1, 0, vocalBuffer, 1, 0, numSamples);
    mixBuffer.addFrom(0, 0, musicStereoBuffer, 0, 0, numSamples);
    mixBuffer.addFrom(1, 0, musicStereoBuffer, 1, 0, numSamples);
    mixBuffer.addFrom(0, 0, soundboardBuffer, 0, 0, numSamples);
    mixBuffer.addFrom(1, 0, soundboardBuffer, 1, 0, numSamples);

    masterProcessor.process(mixBuffer);

    for (int i = 0; i < numOutputChannels; ++i)
        juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);

    if (currentOutputLeft != -1 && juce::isPositiveAndBelow(currentOutputLeft, numOutputChannels))
        juce::FloatVectorOperations::copy(outputChannelData[currentOutputLeft], mixBuffer.getReadPointer(0), numSamples);

    if (currentOutputRight != -1 && juce::isPositiveAndBelow(currentOutputRight, numOutputChannels))
        juce::FloatVectorOperations::copy(outputChannelData[currentOutputRight], mixBuffer.getReadPointer(1), numSamples);
}

void AudioEngine::setVocalInputChannel(int channelIndex)
{
    vocalInputChannel.store(channelIndex);
}

void AudioEngine::setMusicInputChannel(int leftChannelIndex)
{
    musicInputLeftChannel.store(leftChannelIndex);
    musicInputRightChannel.store(leftChannelIndex + 1);
}

void AudioEngine::setSelectedOutputChannels(int leftChannelIndex, int rightChannelIndex)
{
    selectedOutputLeftChannel.store(leftChannelIndex);
    selectedOutputRightChannel.store(rightChannelIndex);
}

juce::String AudioEngine::getVocalInputChannelName() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const int index = vocalInputChannel.load();
        const auto names = device->getInputChannelNames();
        if (juce::isPositiveAndBelow(index, names.size()))
            return names[index];
    }
    return {};
}

juce::String AudioEngine::getMusicInputChannelName() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const int leftIndex = musicInputLeftChannel.load();
        const auto names = device->getInputChannelNames();
        if (juce::isPositiveAndBelow(leftIndex, names.size() - 1))
            return names[leftIndex] + " / " + names[leftIndex + 1];
    }
    return {};
}

juce::String AudioEngine::getSelectedOutputChannelPairName() const
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const int leftIndex = selectedOutputLeftChannel.load();
        const auto names = device->getOutputChannelNames();
        if (juce::isPositiveAndBelow(leftIndex, names.size() - 1))
            return names[leftIndex] + " / " + names[leftIndex + 1];
    }
    return {};
}

void AudioEngine::setVocalInputChannelByName(const juce::String& channelName)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto names = device->getInputChannelNames();
        const int index = names.indexOf(channelName);
        if (index >= 0)
            setVocalInputChannel(index);
    }
}

void AudioEngine::setMusicInputChannelByName(const juce::String& stereoPairName)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto names = device->getInputChannelNames();
        for (int i = 0; i < names.size() - 1; ++i)
        {
            if ((names[i] + " / " + names[i + 1]) == stereoPairName)
            {
                setMusicInputChannel(i);
                return;
            }
        }
    }
}

void AudioEngine::setSelectedOutputChannelsByName(const juce::String& stereoPairName)
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto names = device->getOutputChannelNames();
        for (int i = 0; i < names.size() - 1; ++i)
        {
            if ((names[i] + " / " + names[i + 1]) == stereoPairName)
            {
                setSelectedOutputChannels(i, i + 1);
                return;
            }
        }
    }
}

int AudioEngine::getSelectedOutputLeftChannel() const { return selectedOutputLeftChannel.load(); }
int AudioEngine::getSelectedOutputRightChannel() const { return selectedOutputRightChannel.load(); }

void AudioEngine::linkTrackComponents(TrackComponent& vocalTrackComp, TrackComponent& musicTrackComp)
{
    vocalProcessor.setLevelSource(vocalTrackComp.getLevelMeter().getLevelSource());
    musicProcessor.setLevelSource(musicTrackComp.getLevelMeter().getLevelSource());
    vocalTrackComp.setProcessor(&vocalProcessor);
    musicTrackComp.setProcessor(&musicProcessor);

    vocalTrackComponent = &vocalTrackComp;
    musicTrackComponent = &musicTrackComp;
}

void AudioEngine::linkMasterComponents(MasterUtilityComponent& comp)
{
    masterProcessor.setLevelSource(comp.getMasterLevelMeter().getLevelSource());
    comp.setMasterProcessor(&masterProcessor);
}

TrackProcessor& AudioEngine::getVocalProcessor() { return vocalProcessor; }
TrackProcessor& AudioEngine::getMusicProcessor() { return musicProcessor; }
MasterProcessor& AudioEngine::getMasterProcessor() { return masterProcessor; }

IdolAZ::SoundPlayer& AudioEngine::getSoundPlayer()
{
    return *soundPlayer;
}

void AudioEngine::updateActiveInputChannels(juce::AudioDeviceManager& manager)
{
    auto* currentDevice = manager.getCurrentAudioDevice();
    if (currentDevice == nullptr) return;

    auto activeChannels = manager.getAudioDeviceSetup().inputChannels;
    auto channelNames = currentDevice->getInputChannelNames();

    juce::StringArray activeMonoNames;
    juce::Array<int> activeMonoIndices;
    for (int i = 0; i < channelNames.size(); ++i)
    {
        if (activeChannels[i])
        {
            activeMonoNames.add(channelNames[i]);
            activeMonoIndices.add(i);
        }
    }
    if (vocalTrackComponent != nullptr)
        vocalTrackComponent->populateInputChannels(activeMonoNames, activeMonoIndices);

    juce::StringArray activeStereoNames;
    juce::Array<int> activeStereoStartIndices;
    for (int i = 0; i < channelNames.size() - 1; i += 2)
    {
        if (activeChannels[i] && activeChannels[i + 1])
        {
            activeStereoNames.add(channelNames[i] + " / " + channelNames[i + 1]);
            activeStereoStartIndices.add(i);
        }
    }
    if (musicTrackComponent != nullptr)
        musicTrackComponent->populateInputChannels(activeStereoNames, activeStereoStartIndices);
}