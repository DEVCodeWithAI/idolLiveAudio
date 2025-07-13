/*
  ==============================================================================

    AudioEngine.cpp
    (Fully Functional Send/Return FX Architecture)

  ==============================================================================
*/

#include "AudioEngine.h"
#include "../GUI/Layout/TrackComponent.h"
#include "../GUI/Layout/MasterUtilityComponent.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "SoundPlayer.h"
#include "../GUI/Components/TrackPlayerComponent.h"

AudioEngine::AudioEngine(juce::AudioDeviceManager& manager)
    : deviceManager(manager),
    vocalFxChain(Identifiers::VocalFx1State, Identifiers::VocalFx2State, Identifiers::VocalFx3State, Identifiers::VocalFx4State),
    musicFxChain(Identifiers::MusicFx1State, Identifiers::MusicFx2State, Identifiers::MusicFx3State, Identifiers::MusicFx4State)
{
    formatManager.registerBasicFormats();

    audioRecorder = std::make_unique<AudioRecorder>(formatManager, "");
    vocalTrackRecorder = std::make_unique<AudioRecorder>(formatManager, "Vocal");
    musicTrackRecorder = std::make_unique<AudioRecorder>(formatManager, "Music");
    rawVocalRecorder = std::make_unique<AudioRecorder>(formatManager, "Projects");
    rawMusicRecorder = std::make_unique<AudioRecorder>(formatManager, "Projects");

    soundPlayer = std::make_unique<IdolAZ::SoundPlayer>();

    soundboardMixer.addInputSource(soundPlayer.get(), false);
    directOutputMixer.addInputSource(&playbackSource, false);

}

AudioEngine::~AudioEngine()
{
}

void AudioEngine::prepareAllProcessors(double sampleRate, int samplesPerBlock)
{
    stableSampleRate = sampleRate;
    stableBlockSize = samplesPerBlock;

    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    DBG("AudioEngine preparing all processors with settings: "
        << sampleRate << " Hz, " << samplesPerBlock << " samples.");

    juce::dsp::ProcessSpec stereoSpec{ sampleRate, static_cast<uint32_t>(samplesPerBlock), 2 };

    vocalProcessor.prepare(stereoSpec);
    musicProcessor.prepare(stereoSpec);
    masterProcessor.prepare(stereoSpec);

    for (auto& fxProc : vocalFxChain.processors) fxProc.prepare(stereoSpec);
    for (auto& fxProc : musicFxChain.processors) fxProc.prepare(stereoSpec);

    soundboardMixer.prepareToPlay(samplesPerBlock, sampleRate);
    directOutputMixer.prepareToPlay(samplesPerBlock, sampleRate);
    playbackSource.prepareToPlay(samplesPerBlock, sampleRate);
    vocalTrackSource.prepareToPlay(samplesPerBlock, sampleRate);
    musicTrackSource.prepareToPlay(samplesPerBlock, sampleRate);

    vocalBuffer.setSize(2, samplesPerBlock);
    musicStereoBuffer.setSize(2, samplesPerBlock);
    mixBuffer.setSize(2, samplesPerBlock);
    soundboardBuffer.setSize(2, samplesPerBlock);
    directOutputBuffer.setSize(2, samplesPerBlock);

    fxSendBuffer.setSize(2, samplesPerBlock);
    for (auto& buffer : vocalFxReturnBuffers) buffer.setSize(2, samplesPerBlock);
    for (auto& buffer : musicFxReturnBuffers) buffer.setSize(2, samplesPerBlock);

    vocalProcessor.reset();
    musicProcessor.reset();
    masterProcessor.reset();

    for (auto& fxProc : vocalFxChain.processors) fxProc.reset();
    for (auto& fxProc : musicFxChain.processors) fxProc.reset();
}


void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    prepareAllProcessors(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());

    if (onDeviceStarted)
    {
        juce::MessageManager::callAsync(onDeviceStarted);
    }
}

void AudioEngine::audioDeviceStopped()
{
    vocalProcessor.reset();
    musicProcessor.reset();
    masterProcessor.reset();

    for (auto& fxProc : vocalFxChain.processors) fxProc.reset();
    for (auto& fxProc : musicFxChain.processors) fxProc.reset();

    soundboardMixer.releaseResources();
    directOutputMixer.releaseResources();
    playbackSource.releaseResources();
    vocalTrackSource.releaseResources();
    musicTrackSource.releaseResources();

    currentSampleRate = 0.0;
    currentBlockSize = 0;
    vocalInputChannel.store(-1);
    musicInputLeftChannel.store(-1);
    musicInputRightChannel.store(-1);
    selectedOutputLeftChannel.store(-1);
    selectedOutputRightChannel.store(-1);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    // <<< FIX: Use deviceManager to get the current device safely >>>
    if (auto* currentDevice = deviceManager.getCurrentAudioDevice())
    {
        if (numSamples != currentBlockSize || currentDevice->getCurrentSampleRate() != currentSampleRate)
        {
            prepareAllProcessors(currentDevice->getCurrentSampleRate(), numSamples);
        }
    }

    juce::ScopedNoDenormals noDenormals;

    // --- 1. Xóa tất cả các buffer làm việc ---
    vocalBuffer.clear();
    musicStereoBuffer.clear();
    mixBuffer.clear();
    soundboardBuffer.clear();
    directOutputBuffer.clear();
    for (auto& buffer : vocalFxReturnBuffers) buffer.clear();
    for (auto& buffer : musicFxReturnBuffers) buffer.clear();

    vocalPlayerBuffer.setSize(2, numSamples);
    musicPlayerBuffer.setSize(2, numSamples);
    vocalPlayerBuffer.clear();
    musicPlayerBuffer.clear();


    // --- 2. Lấy tín hiệu từ các Player của Track ---
    juce::AudioSourceChannelInfo vocalPlayerInfo(&vocalPlayerBuffer, 0, numSamples);
    vocalTrackSource.getNextAudioBlock(vocalPlayerInfo);

    juce::AudioSourceChannelInfo musicPlayerInfo(&musicPlayerBuffer, 0, numSamples);
    musicTrackSource.getNextAudioBlock(musicPlayerInfo);


    // --- 3. Xử lý Track Vocal ---
    const int currentVocalIn = vocalInputChannel.load();
    if (juce::isPositiveAndBelow(currentVocalIn, numInputChannels))
    {
        juce::AudioBuffer<float> rawInput(const_cast<float**>(inputChannelData) + currentVocalIn, 1, numSamples);
        vocalBuffer.copyFrom(0, 0, rawInput, 0, 0, numSamples);
        vocalBuffer.copyFrom(1, 0, rawInput, 0, 0, numSamples);
        rawVocalRecorder->processBlock(vocalBuffer, currentSampleRate);
    }
    vocalBuffer.addFrom(0, 0, vocalPlayerBuffer, 0, 0, numSamples);
    vocalBuffer.addFrom(1, 0, vocalPlayerBuffer, 1, 0, numSamples);
    vocalProcessor.process(vocalBuffer);
    vocalTrackRecorder->processBlock(vocalBuffer, currentSampleRate);


    // --- 4. Xử lý Track Music ---
    const int currentMusicLeftIn = musicInputLeftChannel.load();
    const int currentMusicRightIn = musicInputRightChannel.load();
    if (juce::isPositiveAndBelow(currentMusicLeftIn, numInputChannels) &&
        juce::isPositiveAndBelow(currentMusicRightIn, numInputChannels))
    {
        juce::AudioBuffer<float> rawInput(const_cast<float**>(inputChannelData) + currentMusicLeftIn, 2, numSamples);
        musicStereoBuffer.copyFrom(0, 0, rawInput, 0, 0, numSamples);
        musicStereoBuffer.copyFrom(1, 0, rawInput, 1, 0, numSamples);
        rawMusicRecorder->processBlock(musicStereoBuffer, currentSampleRate);
    }
    musicStereoBuffer.addFrom(0, 0, musicPlayerBuffer, 0, 0, numSamples);
    musicStereoBuffer.addFrom(1, 0, musicPlayerBuffer, 1, 0, numSamples);
    musicProcessor.process(musicStereoBuffer);
    musicTrackRecorder->processBlock(musicStereoBuffer, currentSampleRate);


    // --- 5. Lấy tín hiệu từ Soundboard ---
    juce::AudioSourceChannelInfo soundboardChannelInfo(&soundboardBuffer, 0, numSamples);
    soundboardMixer.getNextAudioBlock(soundboardChannelInfo);


    // --- 6. Xử lý FX Sends/Returns ---
    for (int i = 0; i < 4; ++i)
    {
        fxSendBuffer.copyFrom(0, 0, vocalBuffer, 0, 0, numSamples);
        fxSendBuffer.copyFrom(1, 0, vocalBuffer, 1, 0, numSamples);
        fxSendBuffer.applyGain(vocalFxChain.processors[i].getSendLevel());
        vocalFxChain.processors[i].process(fxSendBuffer);
        vocalFxReturnBuffers[i].copyFrom(0, 0, fxSendBuffer, 0, 0, numSamples);
        vocalFxReturnBuffers[i].copyFrom(1, 0, fxSendBuffer, 1, 0, numSamples);
        vocalFxReturnBuffers[i].applyGain(vocalFxChain.processors[i].getReturnLevel());
    }
    for (int i = 0; i < 4; ++i)
    {
        fxSendBuffer.copyFrom(0, 0, musicStereoBuffer, 0, 0, numSamples);
        fxSendBuffer.copyFrom(1, 0, musicStereoBuffer, 1, 0, numSamples);
        fxSendBuffer.applyGain(musicFxChain.processors[i].getSendLevel());
        musicFxChain.processors[i].process(fxSendBuffer);
        musicFxReturnBuffers[i].copyFrom(0, 0, fxSendBuffer, 0, 0, numSamples);
        musicFxReturnBuffers[i].copyFrom(1, 0, fxSendBuffer, 1, 0, numSamples);
        musicFxReturnBuffers[i].applyGain(musicFxChain.processors[i].getReturnLevel());
    }


    // --- 7. Tổng hợp tất cả vào Kênh Master ---
    mixBuffer.clear();

    mixBuffer.copyFrom(0, 0, vocalBuffer, 0, 0, numSamples);
    mixBuffer.copyFrom(1, 0, vocalBuffer, 1, 0, numSamples);

    mixBuffer.addFrom(0, 0, musicStereoBuffer, 0, 0, numSamples);
    mixBuffer.addFrom(1, 0, musicStereoBuffer, 1, 0, numSamples);

    mixBuffer.addFrom(0, 0, soundboardBuffer, 0, 0, numSamples);
    mixBuffer.addFrom(1, 0, soundboardBuffer, 1, 0, numSamples);

    for (int i = 0; i < 4; ++i)
    {
        mixBuffer.addFrom(0, 0, vocalFxReturnBuffers[i], 0, 0, numSamples);
        mixBuffer.addFrom(1, 0, vocalFxReturnBuffers[i], 1, 0, numSamples);
    }
    for (int i = 0; i < 4; ++i)
    {
        mixBuffer.addFrom(0, 0, musicFxReturnBuffers[i], 0, 0, numSamples);
        mixBuffer.addFrom(1, 0, musicFxReturnBuffers[i], 1, 0, numSamples);
    }

    masterProcessor.process(mixBuffer);
    audioRecorder->processBlock(mixBuffer, currentSampleRate);


    // --- 8. Lấy tín hiệu đi thẳng ra Output (Player của REC & PLAY) ---
    juce::AudioSourceChannelInfo directOutputChannelInfo(&directOutputBuffer, 0, numSamples);
    directOutputMixer.getNextAudioBlock(directOutputChannelInfo);


    // --- 9. Gửi tín hiệu cuối cùng ra loa ---
    const int currentOutputLeft = selectedOutputLeftChannel.load();
    const int currentOutputRight = selectedOutputRightChannel.load();
    for (int i = 0; i < numOutputChannels; ++i)
        juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);

    if (juce::isPositiveAndBelow(currentOutputLeft, numOutputChannels))
    {
        juce::FloatVectorOperations::copy(outputChannelData[currentOutputLeft], mixBuffer.getReadPointer(0), numSamples);
        juce::FloatVectorOperations::add(outputChannelData[currentOutputLeft], directOutputBuffer.getReadPointer(0), numSamples);
    }
    if (juce::isPositiveAndBelow(currentOutputRight, numOutputChannels))
    {
        juce::FloatVectorOperations::copy(outputChannelData[currentOutputRight], mixBuffer.getReadPointer(1), numSamples);
        juce::FloatVectorOperations::add(outputChannelData[currentOutputRight], directOutputBuffer.getReadPointer(1), numSamples);
    }
}

TrackProcessor* AudioEngine::getFxProcessorForVocal(int index)
{
    if (juce::isPositiveAndBelow(index, 4))
        return &vocalFxChain.processors[index];
    return nullptr;
}

TrackProcessor* AudioEngine::getFxProcessorForMusic(int index)
{
    if (juce::isPositiveAndBelow(index, 4))
        return &musicFxChain.processors[index];
    return nullptr;
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
}


void AudioEngine::startPlayback(const juce::File& file)
{
    if (!file.existsAsFile()) return;

    playbackSource.stop();
    playbackSource.setSource(nullptr);
    currentPlaybackReader.reset();

    if (auto* reader = formatManager.createReaderFor(file))
    {
        currentPlaybackReader = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        playbackSource.setSource(currentPlaybackReader.get(), 0, nullptr, reader->sampleRate);
        playbackSource.start();
    }
}

void AudioEngine::stopPlayback()
{
    playbackSource.stop();
    playbackSource.setSource(nullptr); // Đẩy source hiện tại ra
    currentPlaybackReader.reset();     // Xóa reader
    playbackSource.setPosition(0);     // Reset vị trí về đầu
}

void AudioEngine::pausePlayback()
{
    playbackSource.stop();
}

void AudioEngine::resumePlayback()
{
    playbackSource.start();
}

void AudioEngine::startTrackPlayback(TrackPlayerComponent::PlayerType type, const juce::File& file)
{
    if (!file.existsAsFile()) return;

    auto* transportToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackSource : &musicTrackSource;
    auto* readerToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackReader : &musicTrackReader;

    transportToUse->stop();
    transportToUse->setSource(nullptr);
    readerToUse->reset();

    if (auto* reader = formatManager.createReaderFor(file))
    {
        *readerToUse = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportToUse->setSource(readerToUse->get(), 0, nullptr, reader->sampleRate);
        transportToUse->start();
    }
}

void AudioEngine::stopTrackPlayback(TrackPlayerComponent::PlayerType type)
{
    auto* transportToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackSource : &musicTrackSource;
    transportToUse->stop();
    transportToUse->setPosition(0);
}

void AudioEngine::pauseTrackPlayback(TrackPlayerComponent::PlayerType type)
{
    auto* transportToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackSource : &musicTrackSource;
    transportToUse->stop();
}

void AudioEngine::resumeTrackPlayback(TrackPlayerComponent::PlayerType type)
{
    auto* transportToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackSource : &musicTrackSource;
    transportToUse->start();
}

void AudioEngine::setTrackPlayerMute(TrackPlayerComponent::PlayerType type, bool shouldBeMuted)
{
    auto* transportToUse = (type == TrackPlayerComponent::PlayerType::Vocal) ? &vocalTrackSource : &musicTrackSource;
    transportToUse->setGain(shouldBeMuted ? 0.0f : 1.0f);
}

juce::AudioTransportSource& AudioEngine::getTrackTransportSource(TrackPlayerComponent::PlayerType type)
{
    return (type == TrackPlayerComponent::PlayerType::Vocal) ? vocalTrackSource : musicTrackSource;
}

AudioRecorder& AudioEngine::getTrackRecorder(TrackPlayerComponent::PlayerType type)
{
    return (type == TrackPlayerComponent::PlayerType::Vocal) ? *vocalTrackRecorder : *musicTrackRecorder;
}

void AudioEngine::startProjectRecording(const juce::String& projectName)
{
    if (isProjectPlaybackMode.load()) return;

    stopProjectRecording();

    currentProjectName = projectName;

    auto projectBaseDir = rawVocalRecorder->getRecordingsDirectory();
    auto projectDir = projectBaseDir.getChildFile(projectName);
    projectDir.createDirectory();

    currentVocalRawFile = projectDir.getChildFile(projectName + "_Vocal_RAW.wav");
    currentMusicRawFile = projectDir.getChildFile(projectName + "_Music_RAW.wav");

    rawVocalRecorder->startRecording(currentVocalRawFile);
    rawMusicRecorder->startRecording(currentMusicRawFile);

    isProjectPlaybackMode = true;
}

void AudioEngine::stopProjectRecording()
{
    if (!isProjectPlaybackMode.load()) return;

    rawVocalRecorder->stop();
    rawMusicRecorder->stop();

    juce::var vocalPath = currentVocalRawFile.getFullPathName();
    juce::var musicPath = currentMusicRawFile.getFullPathName();

    juce::DynamicObject::Ptr projectJson = new juce::DynamicObject();
    projectJson->setProperty("VOCAL_RAW_PATH", vocalPath);
    projectJson->setProperty("MUSIC_RAW_PATH", musicPath);

    auto projectDir = currentVocalRawFile.getParentDirectory();
    juce::File jsonFile = projectDir.getChildFile(currentProjectName + ".json");
    jsonFile.replaceWithText(juce::JSON::toString(juce::var(projectJson.get())));

    isProjectPlaybackMode = false;
    currentProjectName.clear();
}

bool AudioEngine::isProjectRecording() const
{
    return isProjectPlaybackMode.load();
}

namespace ProjectStateIDs
{
    const juce::Identifier name("name");
    const juce::Identifier isPlaying("isPlaying");
}

void AudioEngine::loadProject(const juce::File& projectJsonFile)
{
    auto parsedJson = juce::JSON::parse(projectJsonFile);
    if (!parsedJson.isObject()) return;

    auto vocalPath = parsedJson.getProperty("VOCAL_RAW_PATH", {}).toString();
    auto musicPath = parsedJson.getProperty("MUSIC_RAW_PATH", {}).toString();

    juce::File vocalFile(vocalPath);
    juce::File musicFile(musicPath);

    if (vocalFile.existsAsFile() && musicFile.existsAsFile())
    {
        stopLoadedProject();
        stopPlayback();

        vocalTrackSource.setSource(nullptr);
        musicTrackSource.setSource(nullptr);

        vocalTrackReader.reset();
        musicTrackReader.reset();

        if (auto* reader = formatManager.createReaderFor(vocalFile))
        {
            vocalTrackReader = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            vocalTrackSource.setSource(vocalTrackReader.get(), 0, nullptr, reader->sampleRate);
        }
        if (auto* reader = formatManager.createReaderFor(musicFile))
        {
            musicTrackReader = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            musicTrackSource.setSource(musicTrackReader.get(), 0, nullptr, reader->sampleRate);
        }

        isProjectPlaybackMode = true;
        projectState.setProperty(ProjectStateIDs::name, projectJsonFile.getFileNameWithoutExtension(), nullptr);
        projectState.setProperty(ProjectStateIDs::isPlaying, false, nullptr);

        DBG("Project loaded: " + projectJsonFile.getFileNameWithoutExtension());
    }
}

void AudioEngine::playLoadedProject()
{
    if (!isProjectPlaybackMode) return;
    vocalTrackSource.start();
    musicTrackSource.start();

    projectState.setProperty(ProjectStateIDs::isPlaying, true, nullptr);
}

void AudioEngine::stopLoadedProject()
{
    vocalTrackSource.stop();
    musicTrackSource.stop();

    vocalTrackSource.setSource(nullptr);
    musicTrackSource.setSource(nullptr);
    vocalTrackReader.reset();
    musicTrackReader.reset();

    vocalTrackSource.setPosition(0);
    musicTrackSource.setPosition(0);

    isProjectPlaybackMode = false;

    projectState.setProperty(ProjectStateIDs::name, {}, nullptr);
    projectState.setProperty(ProjectStateIDs::isPlaying, false, nullptr);
}

void AudioEngine::seekProject(double newPositionRatio)
{
    if (isProjectPlaybackMode)
    {
        const double duration = vocalTrackSource.getLengthInSeconds();
        if (duration > 0)
        {
            const double newPosition = duration * newPositionRatio;

            vocalTrackSource.setPosition(newPosition);
            musicTrackSource.setPosition(newPosition);
        }
    }
}

bool AudioEngine::isProjectPlaybackActive() const
{
    return isProjectPlaybackMode.load();
}

juce::ValueTree& AudioEngine::getProjectState()
{
    return projectState;
}