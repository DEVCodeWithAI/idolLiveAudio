/*
  ==============================================================================

    SoundboardComponent.h
    (Refactored for playback control)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/SoundboardManager.h"
#include "../../AudioEngine/AudioEngine.h"


//==============================================================================
class SoundboardComponent : public juce::Component,
    public juce::ChangeListener
{
public:
    SoundboardComponent(AudioEngine& engine);
    ~SoundboardComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void updateTexts();
    void updateButtonLabels();

    AudioEngine& audioEngine;

    // Controls for the soundboard
    juce::Label titleLabel;
    juce::Label volumeLabel;
    juce::Slider volumeSlider;
    juce::ToggleButton enableButton;
    juce::TextButton stopAllButton; // Button to stop all sounds

    // Grid of 9 soundboard buttons
    juce::OwnedArray<juce::TextButton> gridButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundboardComponent)
};