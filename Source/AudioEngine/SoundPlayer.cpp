/*
  ==============================================================================

    SoundPlayer.cpp
    (Implementing user's monophonic playback solution)

  ==============================================================================
*/

#include "SoundPlayer.h"

namespace IdolAZ
{
    //==============================================================================
    class GainAudioSource : public juce::AudioSource
    {
    public:
        GainAudioSource(juce::AudioSource* inputSource, bool deleteInputWhenDeleted)
            : source(inputSource, deleteInputWhenDeleted) {
        }

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
        {
            if (source) source->prepareToPlay(samplesPerBlockExpected, sampleRate);
        }

        void releaseResources() override
        {
            if (source) source->releaseResources();
        }

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
        {
            if (source)
            {
                source->getNextAudioBlock(bufferToFill);
                bufferToFill.buffer->applyGain(gain);
            }
        }

        void setGain(float newGain) { gain = newGain; }
        void clearSource() { source.reset(); }

    private:
        juce::OptionalScopedPointer<juce::AudioSource> source;
        float gain = 1.0f;
    };
    //==============================================================================


    SoundPlayer::SoundPlayer()
    {
        formatManager.registerBasicFormats();

        for (int i = 0; i < numVoices; ++i)
        {
            auto* transportSource = transportSources.add(new juce::AudioTransportSource());
            mixer.addInputSource(transportSource, false);
        }

        gainSource = std::make_unique<GainAudioSource>(&mixer, false);
        gainSource->setGain(0.75f);
    }

    SoundPlayer::~SoundPlayer()
    {
        if (gainSource)
            gainSource->clearSource();

        mixer.removeAllInputs();

        stopAll();

        for (auto* ts : transportSources)
        {
            if (ts != nullptr)
            {
                ts->setSource(nullptr);
            }
        }

        transportSources.clear();
    }

    void SoundPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
    {
        if (gainSource)
            gainSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void SoundPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
    {
        if (gainSource)
            gainSource->getNextAudioBlock(bufferToFill);
    }

    void SoundPlayer::releaseResources()
    {
        if (gainSource)
            gainSource->releaseResources();
    }

    // Applying your correct logic for monophonic playback.
    void SoundPlayer::play(const juce::File& audioFile, int slotIndex)
    {
        if (!enabled.load())
            return;

        if (!juce::isPositiveAndBelow(slotIndex, numVoices))
        {
            jassertfalse;
            return;
        }

        if (!audioFile.existsAsFile())
        {
            DBG("SoundPlayer::play() - File does not exist: " << audioFile.getFullPathName());
            return;
        }

        // As you suggested, stop all other sounds to enforce monophonic behavior.
        stopAll();

        auto* transportSource = transportSources[slotIndex];

        // Safely decouple the transport from its old audio source.
        transportSource->setSource(nullptr);

        // Create a new reader for the audio file.
        if (auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(audioFile)))
        {
            // Create a new reader source and assign it to the unique_ptr for this slot.
            readerSources[slotIndex] = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);

            // Pass the raw pointer to the transport source.
            transportSource->setSource(readerSources[slotIndex].get(), 0, nullptr, readerSources[slotIndex]->getAudioFormatReader()->sampleRate);

            transportSource->setPosition(0.0);
            transportSource->start();

            DBG("Playing sound on slot " << slotIndex << ": " << audioFile.getFileName());
        }
        else
        {
            DBG("SoundPlayer::play() - Could not create reader for file: " << audioFile.getFullPathName());
        }
    }

    void SoundPlayer::stopAll()
    {
        for (auto* source : transportSources)
        {
            if (source != nullptr && source->isPlaying())
                source->stop();
        }
    }

    void SoundPlayer::setGain(float newGain)
    {
        if (gainSource)
            gainSource->setGain(newGain);
    }

    void SoundPlayer::setEnabled(bool shouldBeEnabled)
    {
        enabled = shouldBeEnabled;
        if (!enabled)
        {
            stopAll();
        }
    }

} // namespace IdolAZ