#pragma once

#include <JuceHeader.h>
#include "../../Data/PresetManager.h"

class PresetManagerWindow : public juce::DocumentWindow
{
public:
    PresetManagerWindow();
    ~PresetManagerWindow() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManagerWindow)
};