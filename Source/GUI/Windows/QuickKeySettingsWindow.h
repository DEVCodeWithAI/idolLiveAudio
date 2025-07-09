/*
  ==============================================================================

    QuickKeySettingsWindow.h
    (Fixed scope issue for bounds variables)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Data/SoundboardSlot.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/SoundboardManager.h"
#include <functional>

//==============================================================================
/** A DocumentWindow to host the settings panel. */
class QuickKeySettingsWindow : public juce::DocumentWindow
{
public:
    QuickKeySettingsWindow(std::function<void()> onWindowClosed);
    ~QuickKeySettingsWindow() override;

    void closeButtonPressed() override;

private:
    std::function<void()> onWindowClosedCallback;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickKeySettingsWindow)
};


//==============================================================================
/** The main content component inside the QuickKeySettingsWindow. */
class QuickKeySettingsContentComponent : public juce::Component,
    public juce::Button::Listener,
    public juce::ChangeListener,
    public juce::TextEditor::Listener
{
public:
    QuickKeySettingsContentComponent();
    ~QuickKeySettingsContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void showDetailsForSlot(int slotIndex);
    void chooseFileForSlot(int slotIndex);
    void setHotkeyForSlot(int slotIndex);
    void saveChanges();
    void updateAllUITexts();

    // Data model
    juce::Array<SoundboardSlot> slotsToEdit;
    int selectedSlot = 0;

    // Left Panel: Slot selection
    juce::OwnedArray<juce::TextButton> slotGridButtons;

    // Right Panel: Slot editor
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::TextButton assignFileButton;
    juce::TextButton setHotkeyButton;

    // Right Panel: Profile management
    juce::TextButton saveButton, cleanButton, importButton, exportButton;

    // <<< FIX IS HERE: Declare bounds as member variables >>>
    juce::Rectangle<int> editorGroupBounds;
    juce::Rectangle<int> profileGroupBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickKeySettingsContentComponent)
};