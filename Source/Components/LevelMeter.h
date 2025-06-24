/*
  ==============================================================================

    LevelMeter.h
    Created: 11 Jun 2025 8:39:17pm
    Author:  Kevin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class LevelMeter : public juce::Component,
    public juce::Timer
{
public:
    LevelMeter();
    ~LevelMeter() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;
    void setLevel(float newLevel);

    // *** ADD THIS FUNCTION ***
    std::atomic<float>* getLevelSource() { return &level; }

private:
    std::atomic<float> level;

    // --- BIẾN CHO GIAO DIỆN HIỆN ĐẠI ---
    float smoothedLevel = 0.0f;
    float peakLevel = 0.0f;
    int peakHoldCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};