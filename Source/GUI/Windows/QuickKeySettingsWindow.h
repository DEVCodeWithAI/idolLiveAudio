/*
  ==============================================================================

    QuickKeySettingsWindow.h
    (Build Fix 02)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Data/SoundboardSlot.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/SoundboardManager.h"

class QuickKeySettingsContentComponent; // Forward declaration

//==============================================================================
/** A DocumentWindow to host the settings panel. */
class QuickKeySettingsWindow : public juce::DocumentWindow
{
public:
    QuickKeySettingsWindow();
    ~QuickKeySettingsWindow() override;

    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickKeySettingsWindow)
};


//==============================================================================
/** A dedicated component for editing a single soundboard slot. */
class SoundboardSlotEditor : public juce::Component,
    public juce::Button::Listener,
    public juce::TextEditor::Listener
{
public:
    SoundboardSlotEditor(SoundboardSlot& slotToEdit);
    ~SoundboardSlotEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Callbacks for parent component
    std::function<void()> onAssignFile;
    std::function<void()> onSetHotkey;

    void updateLabels();

private:
    void buttonClicked(juce::Button* button) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;

    SoundboardSlot& slot; // A reference to the data model

    juce::Label nameLabel, fileLabel, hotkeyLabel;
    juce::TextEditor nameEditor;
    juce::TextButton assignFileButton, setHotkeyButton;
}; // <<< --- FIX: ĐÃ THÊM DẤU CHẤM PHẨY (;) BẮT BUỘC VÀO ĐÂY ---


//==============================================================================
/** The main content component inside the QuickKeySettingsWindow. */
class QuickKeySettingsContentComponent : public juce::Component,
    public juce::Button::Listener,
    public juce::ChangeListener
{
public:
    QuickKeySettingsContentComponent();
    ~QuickKeySettingsContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    void chooseFileForSlot(int slotIndex);
    void setHotkeyForSlot(int slotIndex);
    void saveChanges();
    void updateAllSlotEditors();

    juce::Array<SoundboardSlot> slotsToEdit;
    juce::OwnedArray<SoundboardSlotEditor> slotEditors;

    juce::TextButton saveButton, cleanButton, importButton, exportButton;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickKeySettingsContentComponent)
};