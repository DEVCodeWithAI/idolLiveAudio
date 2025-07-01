// Source/GUI/Layout/PluginManagementComponent.h

#pragma once

#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/PluginManager/PluginManager.h"

// Forward declare the helper class
class ScanningWindow;

// <<< MODIFIED: Inherit from juce::Timer to perform non-blocking scan >>>
class PluginManagementComponent : public juce::Component,
    public juce::ChangeListener,
    public juce::Timer
{
public:
    PluginManagementComponent();
    ~PluginManagementComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // <<< NEW: Timer callback for scanning >>>
    void timerCallback() override;

private:
    void updateTexts();

    // <<< NEW: Helper to start the scan >>>
    void startScan();

    juce::TextButton scanPluginsButton, managePluginsButton, addPluginButton;

    // <<< NEW: State variables for the non-blocking scan >>>
    std::unique_ptr<ScanningWindow> scanningWindow;
    juce::Array<juce::File> filesToScan;
    int nextFileToScan = 0;
    juce::KnownPluginList tempList; // A temporary list to build plugins into
};