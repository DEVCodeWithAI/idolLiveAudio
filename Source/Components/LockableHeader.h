/*
  ==============================================================================

    LockableHeader.h
    Created: 11 Jun 2025 8:39:27pm
    Author:  Kevin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class LockableHeader  : public juce::Component
{
public:
    LockableHeader();
    ~LockableHeader() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LockableHeader)
};
