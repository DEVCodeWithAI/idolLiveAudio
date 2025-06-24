/*
  ==============================================================================

    PluginManagerWindow.h
    (FIXED)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Data/PluginManager/PluginManager.h"
#include "../../Data/LanguageManager/LanguageManager.h"

class PluginManagerContentComponent : public juce::Component,
    public juce::ListBoxModel,
    public juce::ChangeListener
{
public:
    PluginManagerContentComponent();
    ~PluginManagerContentComponent() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
private:
    void updateTexts();
    void removeSelectedPlugin();
    juce::ListBox pluginListBox;
    juce::TextButton scanButton, addButton, removeButton;
    juce::Label titleLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerContentComponent)
};

class PluginManagerWindow : public juce::DocumentWindow
{
public:
    PluginManagerWindow();
    ~PluginManagerWindow() override;

    void closeButtonPressed() override;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerWindow)
};