#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/PluginManager/PluginManager.h"

class PluginManagementComponent : public juce::Component, public juce::ChangeListener {
public:
    PluginManagementComponent();
    ~PluginManagementComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void updateTexts();

    juce::TextButton scanPluginsButton, managePluginsButton, addPluginButton;
};