#pragma once

#include <JuceHeader.h>
#include "../../Data/SoundboardSlot.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Data/SoundboardManager.h"
#include <functional>

// Forward declaration
class QuickKeySettingsContentComponent;

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


// <<< MODIFIED: Inherits from juce::Timer instead of juce::FocusListener >>>
class QuickKeySettingsContentComponent : public juce::Component,
    public juce::Button::Listener,
    public juce::ChangeListener,
    public juce::TextEditor::Listener,
    public juce::Timer // Changed from FocusListener
{
public:
    QuickKeySettingsContentComponent();
    ~QuickKeySettingsContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // <<< ADDED: Declaration for the timer callback >>>
    void timerCallback() override;

private:
    void showDetailsForSlot(int slotIndex);
    void chooseFileForSlot(int slotIndex);
    void setHotkeyForSlot(int slotIndex);
    void saveChanges();
    void updateAllUITexts();
    void cancelCaptureMode();

    juce::Array<SoundboardSlot> slotsToEdit;
    int selectedSlot = 0;
    bool isCapturingHotkey = false;

    juce::OwnedArray<juce::TextButton> slotGridButtons;
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::TextButton assignFileButton;
    juce::TextButton setHotkeyButton;
    juce::TextButton saveButton, cleanButton, importButton, exportButton;

    juce::Rectangle<int> editorGroupBounds;
    juce::Rectangle<int> profileGroupBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickKeySettingsContentComponent)
};