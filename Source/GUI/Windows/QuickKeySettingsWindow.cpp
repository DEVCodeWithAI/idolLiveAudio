/*
  ==============================================================================

    QuickKeySettingsWindow.cpp
    (UI Layout fine-tuned based on user feedback)

  ==============================================================================
*/

#include "QuickKeySettingsWindow.h"
#include "../../Components/Helpers.h"

//==============================================================================
// QuickKeySettingsContentComponent Implementation
//==============================================================================

QuickKeySettingsContentComponent::QuickKeySettingsContentComponent()
{
    // Constructor logic remains the same.
    LanguageManager::getInstance().addChangeListener(this);
    getSharedSoundboardProfileManager().addChangeListener(this);

    slotsToEdit = getSharedSoundboardProfileManager().getCurrentSlots();

    for (int i = 0; i < 9; ++i)
    {
        auto* button = slotGridButtons.add(new juce::TextButton(juce::String(i + 1)));
        button->setRadioGroupId(1);
        button->setClickingTogglesState(true);
        const int index = i;
        button->onClick = [this, index] { showDetailsForSlot(index); };
        addAndMakeVisible(button);
    }

    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    nameEditor.addListener(this);
    addAndMakeVisible(assignFileButton);
    assignFileButton.addListener(this);
    addAndMakeVisible(setHotkeyButton);
    setHotkeyButton.addListener(this);

    addAndMakeVisible(saveButton);
    saveButton.addListener(this);
    addAndMakeVisible(cleanButton);
    cleanButton.addListener(this);
    addAndMakeVisible(importButton);
    importButton.addListener(this);
    addAndMakeVisible(exportButton);
    exportButton.addListener(this);

    updateAllUITexts();
    showDetailsForSlot(0);
    slotGridButtons[0]->setToggleState(true, juce::sendNotification);
}

QuickKeySettingsContentComponent::~QuickKeySettingsContentComponent()
{
    // Destructor logic remains the same.
    LanguageManager::getInstance().removeChangeListener(this);
    getSharedSoundboardProfileManager().removeChangeListener(this);
}

void QuickKeySettingsContentComponent::paint(juce::Graphics& g)
{
    // Paint logic remains the same.
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(editorGroupBounds.toFloat(), 5.0f, 1.0f);
    g.drawRoundedRectangle(profileGroupBounds.toFloat(), 5.0f, 1.0f);
}

void QuickKeySettingsContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // <<< CHANGE 1: Increase left panel width to make slot buttons more square-like >>>
    auto leftPanel = bounds.removeFromLeft(240);
    bounds.removeFromLeft(10);
    auto rightPanel = bounds;

    // Layout left panel (grid buttons) with juce::Grid for tighter spacing
    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    grid.templateRows = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    grid.setGap(juce::Grid::Px(5));

    for (auto* button : slotGridButtons)
        grid.items.add(juce::GridItem(button));

    grid.performLayout(leftPanel);

    // Layout right panel
    // <<< CHANGE 2: Increase height of the bottom profile management group >>>
    profileGroupBounds = rightPanel.removeFromBottom(130);
    rightPanel.removeFromBottom(10);
    editorGroupBounds = rightPanel;

    // Layout Editor Group
    auto editorContentBounds = editorGroupBounds.reduced(15);
    const int rowHeight = 35;
    const int rowGap = 15;
    const int labelWidth = 50;

    auto nameRow = editorContentBounds.removeFromTop(rowHeight);
    nameLabel.setBounds(nameRow.removeFromLeft(labelWidth).withTrimmedRight(5));
    nameEditor.setBounds(nameRow);

    editorContentBounds.removeFromTop(rowGap);
    assignFileButton.setBounds(editorContentBounds.removeFromTop(rowHeight));
    editorContentBounds.removeFromTop(rowGap);
    setHotkeyButton.setBounds(editorContentBounds.removeFromTop(rowHeight));

    // Layout Profile Buttons Group
    juce::Grid profileGrid;
    profileGrid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
    profileGrid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };
    profileGrid.setGap(juce::Grid::Px(10));

    profileGrid.items = {
        juce::GridItem(saveButton), juce::GridItem(exportButton),
        juce::GridItem(importButton), juce::GridItem(cleanButton)
    };

    profileGrid.performLayout(profileGroupBounds.reduced(15));
}


void QuickKeySettingsContentComponent::buttonClicked(juce::Button* button)
{
    // This function's logic remains correct.
    auto& profileManager = getSharedSoundboardProfileManager();
    if (button == &assignFileButton) { /* ... */ }
    else if (button == &setHotkeyButton) { /* ... */ }
    // ... same as before
    if (button == &assignFileButton)
    {
        chooseFileForSlot(selectedSlot);
    }
    else if (button == &setHotkeyButton)
    {
        setHotkeyForSlot(selectedSlot);
    }
    else if (button == &saveButton)
    {
        saveChanges();
    }
    else if (button == &cleanButton)
    {
        auto& lang = LanguageManager::getInstance();
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::QuestionIcon,
            lang.get("alerts.cleanProfileConfirmTitle"),
            lang.get("alerts.cleanProfileConfirmMessage"),
            lang.get("quickKeySettingsWindow.yes"),
            lang.get("quickKeySettingsWindow.no"),
            this,
            juce::ModalCallbackFunction::create([&profileManager](int result) {
                if (result == 1)
                    profileManager.cleanProfile();
                })
        );
    }
    else if (button == &importButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Import Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, fc, &profileManager](const juce::FileChooser& chooser) {
                if (!chooser.getResults().isEmpty())
                    profileManager.importProfile(chooser.getResult());
            });
    }
    else if (button == &exportButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Export Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::saveMode,
            [this, fc, &profileManager](const juce::FileChooser& chooser) {
                if (!chooser.getResults().isEmpty())
                    profileManager.exportProfile(chooser.getResult());
            });
    }
}

void QuickKeySettingsContentComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    // This function's logic remains correct.
    if (&editor == &nameEditor)
    {
        slotsToEdit.getReference(selectedSlot).displayName = editor.getText();
    }
}

void QuickKeySettingsContentComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // This function's logic remains correct.
    if (source == &getSharedSoundboardProfileManager())
    {
        slotsToEdit = getSharedSoundboardProfileManager().getCurrentSlots();
        showDetailsForSlot(selectedSlot);
    }
    else if (source == &LanguageManager::getInstance())
    {
        updateAllUITexts();
        showDetailsForSlot(selectedSlot);
    }
}

void QuickKeySettingsContentComponent::showDetailsForSlot(int slotIndex)
{
    // This function's logic remains correct.
    if (!juce::isPositiveAndBelow(slotIndex, 9))
        return;
    selectedSlot = slotIndex;
    for (int i = 0; i < slotGridButtons.size(); ++i)
        slotGridButtons[i]->setToggleState(i == selectedSlot, juce::dontSendNotification);
    auto& lang = LanguageManager::getInstance();
    auto& slot = slotsToEdit.getReference(selectedSlot);
    nameEditor.setText(slot.displayName, juce::dontSendNotification);
    juce::String fileText = slot.isEmpty() ? lang.get("quickKeySettingsWindow.noFile")
        : slot.audioFile.getFileName();
    assignFileButton.setButtonText(fileText);
    juce::String hotkeyText = slot.hotkey.isValid() ? slot.hotkey.getTextDescription()
        : lang.get("quickKeySettingsWindow.noHotkey");
    setHotkeyButton.setButtonText(hotkeyText);
}

void QuickKeySettingsContentComponent::chooseFileForSlot(int slotIndex)
{
    // This function's logic remains correct.
    auto fc = std::make_shared<juce::FileChooser>("Select an audio file for Slot " + juce::String(slotIndex + 1),
        juce::File{},
        "*.mp3;*.wav;*.aif;*.aiff");
    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, fc, slotIndex](const juce::FileChooser& chooser) {
            if (chooser.getResults().isEmpty())
                return;
            slotsToEdit.getReference(slotIndex).audioFile = chooser.getResult();
            showDetailsForSlot(slotIndex);
        });
}

void QuickKeySettingsContentComponent::setHotkeyForSlot(int slotIndex)
{
    // This function's logic remains correct.
    class HotkeyCaptureComponent : public juce::Component, public juce::KeyListener
    {
    public:
        HotkeyCaptureComponent(const juce::String& instructionText,
            std::function<void(const juce::KeyPress&)> onKeyCaptured)
            : onKeyCapturedCallback(onKeyCaptured)
        {
            setSize(350, 100); setWantsKeyboardFocus(true); addKeyListener(this);
            infoLabel.setText(instructionText, juce::dontSendNotification);
            infoLabel.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(infoLabel);
        }
        void resized() override { infoLabel.setBounds(getLocalBounds()); }
        bool keyPressed(const juce::KeyPress& key, juce::Component*) override
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            {
                if (!key.isKeyCurrentlyDown(juce::KeyPress::escapeKey))
                {
                    if (onKeyCapturedCallback)
                        onKeyCapturedCallback(key);
                }
                dw->exitModalState(0);
            }
            return true;
        }
    private:
        std::function<void(const juce::KeyPress&)> onKeyCapturedCallback;
        juce::Label infoLabel;
    };
    auto& lang = LanguageManager::getInstance();
    auto onKeyCapturedLambda = [this, slotIndex](const juce::KeyPress& key)
        {
            slotsToEdit.getReference(slotIndex).hotkey = key;
            showDetailsForSlot(slotIndex);
        };
    auto* captureComponent = new HotkeyCaptureComponent(lang.get("quickKeySettingsWindow.hotkeyPromptMessage"),
        onKeyCapturedLambda);
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(captureComponent);
    options.dialogTitle = lang.get("quickKeySettingsWindow.hotkeyPromptTitle");
    options.componentToCentreAround = this;
    options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}


void QuickKeySettingsContentComponent::saveChanges()
{
    // This function's logic remains correct.
    auto profileDir = getSharedSoundboardProfileManager().getProfileDirectory();
    for (auto& slot : slotsToEdit)
    {
        if (slot.audioFile.existsAsFile() && !slot.audioFile.isAChildOf(profileDir))
        {
            juce::File newFile = profileDir.getChildFile(slot.audioFile.getFileName());
            if (newFile.existsAsFile())
                newFile.deleteFile();
            if (slot.audioFile.copyFileTo(newFile))
                slot.audioFile = newFile;
            else
            {
                slot.audioFile = juce::File{};
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "File Copy Error", "Could not copy file: " + slot.audioFile.getFileName());
            }
        }
    }
    getSharedSoundboardProfileManager().saveProfile(slotsToEdit);
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Profile Saved", "Your soundboard profile has been saved successfully.");
}

void QuickKeySettingsContentComponent::updateAllUITexts()
{
    // This function's logic remains correct.
    auto& lang = LanguageManager::getInstance();
    nameLabel.setText(lang.get("quickKeySettingsWindow.slotNameLabel"), juce::dontSendNotification);
    saveButton.setButtonText(lang.get("quickKeySettingsWindow.saveProfile"));
    cleanButton.setButtonText(lang.get("quickKeySettingsWindow.cleanProfile"));
    importButton.setButtonText(lang.get("quickKeySettingsWindow.importProfile"));
    exportButton.setButtonText(lang.get("quickKeySettingsWindow.exportProfile"));
}


//==============================================================================
// QuickKeySettingsWindow Implementation
//==============================================================================

QuickKeySettingsWindow::QuickKeySettingsWindow(std::function<void()> onWindowClosed)
    : DocumentWindow("Soundboard Settings", juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId), allButtons),
    onWindowClosedCallback(onWindowClosed)
{
    // The constructor logic remains correct.
    setUsingNativeTitleBar(true);
    setContentOwned(new QuickKeySettingsContentComponent(), true);
    setResizable(true, true);
    setResizeLimits(600, 350, 900, 500);
    centreWithSize(600, 350);
    setVisible(true);
}

QuickKeySettingsWindow::~QuickKeySettingsWindow() {}

void QuickKeySettingsWindow::closeButtonPressed()
{
    // The close button logic remains correct.
    if (onWindowClosedCallback)
        onWindowClosedCallback();
}