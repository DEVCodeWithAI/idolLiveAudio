/*
  ==============================================================================

    SoundPlayer.h
    (Final version using user's unique_ptr solution)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace IdolAZ
{
    class GainAudioSource;

    class SoundPlayer : public juce::AudioSource
    {
    public:
        SoundPlayer();
        ~SoundPlayer() override;

        void play(const juce::File& audioFile, int slotIndex);
        void stopAll();
        void setGain(float newGain);
        void setEnabled(bool shouldBeEnabled);

        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
        void releaseResources() override;

    private:
        static constexpr int numVoices = 9;

        juce::AudioFormatManager formatManager;
        juce::MixerAudioSource mixer;

        std::unique_ptr<GainAudioSource> gainSource;
        juce::OwnedArray<juce::AudioTransportSource> transportSources;

        // Using a unique_ptr to explicitly manage the lifetime of the reader sources.
        // This is a robust solution to prevent memory leaks.
        // We use an array of unique_ptrs to manage each slot's source independently.
        std::array<std::unique_ptr<juce::AudioFormatReaderSource>, numVoices> readerSources;

        std::atomic<bool> enabled{ true };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundPlayer)
    };

} // namespace IdolAZ