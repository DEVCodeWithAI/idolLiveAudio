#pragma once

#include <JuceHeader.h>
#include "../../AudioEngine/MasterProcessor.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../Layout/TrackComponent.h" 

class MasterPluginsWindow : public juce::DocumentWindow
{
public:
    MasterPluginsWindow(MasterProcessor& processor, std::function<void()> onWindowClosed);
    ~MasterPluginsWindow() override;

    void closeButtonPressed() override;

private:
    std::function<void()> onWindowClosedCallback;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterPluginsWindow)
};