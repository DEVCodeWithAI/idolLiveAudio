#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A custom component that draws the splash screen image and status text.
    This is the *content* that will be placed inside a DocumentWindow.
*/
class SplashScreenComponent : public juce::Component
{
public:
    SplashScreenComponent();
    ~SplashScreenComponent() override;

    void paint(juce::Graphics& g) override;

    /** Updates the status message shown on the splash screen. */
    void setStatusMessage(const juce::String& message);

private:
    juce::Image splashImage;
    juce::String statusMessage{ "Initializing..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreenComponent)
};