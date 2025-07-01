#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../AudioEngine/ProcessorBase.h"

//==============================================================================
/*
    This class represents the pop-up window that appears when a user clicks on
    an FX button. It contains the plugin list, send/return faders, and other
    controls for a single FX chain.
*/
class FXChainWindow : public juce::DocumentWindow
{
public:
    // <<< MODIFIED: Forward-declare Content class in the public section >>>
    class Content;

    FXChainWindow(const juce::String& name, ProcessorBase& processorToControl);
    ~FXChainWindow() override;

    void closeButtonPressed() override;

private:
    std::unique_ptr<Content> contentComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXChainWindow)
};