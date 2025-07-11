#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include <functional>

class BeatManagerComponent : public juce::Component,
    private juce::ChangeListener,
    private juce::TextEditor::Listener,
    private juce::Button::Listener,
    private juce::ListBoxModel
{
public:
    BeatManagerComponent();
    ~BeatManagerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Callbacks to communicate with the parent component
    std::function<void()> onWantsToExpand;
    std::function<void()> onBeatPlayed;

private:
    // Forward declare the custom list box item and the custom search editor
    class BeatItemComponent;
    class SearchEditor;

    // UI Elements
    juce::Label titleLabel;
    juce::Label searchLabel;
    std::unique_ptr<SearchEditor> searchEditor;
    juce::TextButton searchButton;
    juce::TextButton openFolderButton;
    juce::TextButton rescanButton;
    juce::TextButton changeFolderButton;

    juce::ListBox searchResultsBox;

    // Data
    juce::File beatFolder; // Thư mục mặc định
    juce::File customScanFolder; // Thư mục tùy chỉnh tạm thời
    juce::File beatDatabaseFile;
    juce::Array<juce::var> allBeats;
    juce::Array<juce::var> filteredBeats;

    // UI Callbacks
    void buttonClicked(juce::Button* button) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void deleteBeat(int rowNumber);

    // ListBoxModel Overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;

    // Helper Functions
    void initialisePaths();
    void updateTexts();
    void rescanBeats(juce::File folderToScan = {});
    void loadBeats();
    void filterBeats();
    void playBeat(const juce::File& fileToPlay);
};