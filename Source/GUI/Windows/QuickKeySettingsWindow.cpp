/*
  ==============================================================================

    QuickKeySettingsWindow.cpp
    (Fixed state management and memory corruption bug)

  ==============================================================================
*/

#include "QuickKeySettingsWindow.h"
#include "../../Components/Helpers.h"

// ... SoundboardSlotEditor Implementation (no changes needed here) ...
SoundboardSlotEditor::SoundboardSlotEditor(SoundboardSlot& slotToEdit) : slot(slotToEdit)
{
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    nameEditor.setText(slot.displayName, juce::dontSendNotification);
    nameEditor.addListener(this);

    addAndMakeVisible(fileLabel);
    addAndMakeVisible(assignFileButton);
    assignFileButton.addListener(this);

    addAndMakeVisible(hotkeyLabel);
    addAndMakeVisible(setHotkeyButton);
    setHotkeyButton.addListener(this);

    updateLabels();
}

SoundboardSlotEditor::~SoundboardSlotEditor() {}

void SoundboardSlotEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

void SoundboardSlotEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    const int nameRowHeight = 24;
    const int buttonRowHeight = 28;
    const int paddingY_small = 8;
    const int paddingY_large = 15;
    const int labelWidth = 80;

    const int totalContentHeight = nameRowHeight + paddingY_large + (buttonRowHeight * 2) + paddingY_small;
    auto centeredBounds = bounds.withHeight(totalContentHeight).withY(bounds.getCentreY() - totalContentHeight / 2);

    auto nameArea = centeredBounds.removeFromTop(nameRowHeight);
    nameLabel.setBounds(nameArea.removeFromLeft(labelWidth));
    nameEditor.setBounds(nameArea);

    centeredBounds.removeFromTop(paddingY_large);

    auto fileArea = centeredBounds.removeFromTop(buttonRowHeight);
    fileLabel.setBounds(fileArea.removeFromLeft(labelWidth));
    assignFileButton.setBounds(fileArea);

    centeredBounds.removeFromTop(paddingY_small);

    auto hotkeyArea = centeredBounds.removeFromTop(buttonRowHeight);
    hotkeyLabel.setBounds(hotkeyArea.removeFromLeft(labelWidth));
    setHotkeyButton.setBounds(hotkeyArea);
}

void SoundboardSlotEditor::updateLabels()
{
    auto& lang = LanguageManager::getInstance();
    nameLabel.setText(lang.get("quickKeySettingsWindow.slotNameLabel"), juce::dontSendNotification);

    fileLabel.setText(lang.get("quickKeySettingsWindow.fileLabel"), juce::dontSendNotification);
    juce::String fileText = slot.isEmpty() ? lang.get("quickKeySettingsWindow.noFile")
        : slot.audioFile.getFileName();
    assignFileButton.setButtonText(fileText);

    hotkeyLabel.setText(lang.get("quickKeySettingsWindow.hotkeyLabel"), juce::dontSendNotification);
    juce::String hotkeyText = slot.hotkey.isValid() ? slot.hotkey.getTextDescription()
        : lang.get("quickKeySettingsWindow.noHotkey");
    setHotkeyButton.setButtonText(hotkeyText);

    // Also update the name editor in case it was changed externally (e.g., by Clean Profile)
    nameEditor.setText(slot.displayName, juce::dontSendNotification);
}

void SoundboardSlotEditor::buttonClicked(juce::Button* button)
{
    if (button == &assignFileButton && onAssignFile)
        onAssignFile();
    else if (button == &setHotkeyButton && onSetHotkey)
        onSetHotkey();
}

void SoundboardSlotEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &nameEditor)
        slot.displayName = editor.getText();
}


//==============================================================================
// QuickKeySettingsContentComponent Implementation
//==============================================================================

QuickKeySettingsContentComponent::QuickKeySettingsContentComponent()
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedSoundboardProfileManager().addChangeListener(this);

    // Make a safe, deep copy of the slots to edit.
    slotsToEdit = getSharedSoundboardProfileManager().getCurrentSlots();

    for (int i = 0; i < 9; ++i)
    {
        // Give each editor a reference to an element in our own 'slotsToEdit' array.
        auto& slot = slotsToEdit.getReference(i);
        auto* editor = slotEditors.add(new SoundboardSlotEditor(slot));

        editor->onAssignFile = [this, i] { chooseFileForSlot(i); };
        editor->onSetHotkey = [this, i] { setHotkeyForSlot(i); };

        addAndMakeVisible(editor);
    }

    addAndMakeVisible(saveButton);
    saveButton.addListener(this);

    addAndMakeVisible(cleanButton);
    cleanButton.addListener(this);

    addAndMakeVisible(importButton);
    importButton.addListener(this);

    addAndMakeVisible(exportButton);
    exportButton.addListener(this);

    updateAllSlotEditors();
}

QuickKeySettingsContentComponent::~QuickKeySettingsContentComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    getSharedSoundboardProfileManager().removeChangeListener(this);
}

void QuickKeySettingsContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void QuickKeySettingsContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    const int columns = 3;
    const int spacing = 10;
    const int itemHeight = 110;
    const int rows = 3;
    const int totalSpacingY = (rows - 1) * spacing;
    const int gridHeight = rows * itemHeight + totalSpacingY;

    juce::Rectangle<int> gridArea = bounds.removeFromTop(gridHeight);
    bounds.removeFromTop(20);

    const int itemWidth = (gridArea.getWidth() - (columns - 1) * spacing) / columns;

    for (int i = 0; i < slotEditors.size(); ++i)
    {
        int col = i % columns;
        int row = i / columns;
        int itemX = gridArea.getX() + col * (itemWidth + spacing);
        int itemY = gridArea.getY() + row * (itemHeight + spacing);
        slotEditors[i]->setBounds(itemX, itemY, itemWidth, itemHeight);
    }

    const int buttonHeight = 38;
    juce::FlexBox bottomFb;
    bottomFb.flexDirection = juce::FlexBox::Direction::row;
    bottomFb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    bottomFb.alignItems = juce::FlexBox::AlignItems::center;
    bottomFb.items.add(juce::FlexItem(saveButton).withMinWidth(130).withHeight(buttonHeight));
    bottomFb.items.add(juce::FlexItem(cleanButton).withMinWidth(130).withHeight(buttonHeight));
    bottomFb.items.add(juce::FlexItem(importButton).withMinWidth(130).withHeight(buttonHeight));
    bottomFb.items.add(juce::FlexItem(exportButton).withMinWidth(130).withHeight(buttonHeight));
    bottomFb.performLayout(bounds.removeFromTop(buttonHeight));
}


void QuickKeySettingsContentComponent::buttonClicked(juce::Button* button)
{
    auto& profileManager = getSharedSoundboardProfileManager();

    if (button == &saveButton)
    {
        saveChanges();
    }
    else if (button == &cleanButton)
    {
        auto& lang = LanguageManager::getInstance();
        auto* alertWindow = new juce::AlertWindow(
            lang.get("alerts.cleanProfileConfirmTitle"),
            lang.get("alerts.cleanProfileConfirmMessage"),
            juce::AlertWindow::QuestionIcon,
            this
        );

        alertWindow->addButton(lang.get("quickKeySettingsWindow.yes"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alertWindow->addButton(lang.get("quickKeySettingsWindow.no"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, &profileManager](int result)
            {
                if (result == 1)
                {
                    profileManager.cleanProfile();
                }
            }
        ), true);
    }
    else if (button == &importButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Import Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, fc](const juce::FileChooser& chooser)
            {
                if (chooser.getResults().isEmpty())
                    return;
                getSharedSoundboardProfileManager().importProfile(chooser.getResult());
            });
    }
    else if (button == &exportButton)
    {
        auto fc = std::make_shared<juce::FileChooser>("Export Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::saveMode,
            [this, fc](const juce::FileChooser& chooser)
            {
                if (chooser.getResults().isEmpty())
                    return;
                getSharedSoundboardProfileManager().exportProfile(chooser.getResult());
            });
    }
}

// <<< THIS IS THE CRITICAL FIX >>>
void QuickKeySettingsContentComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &getSharedSoundboardProfileManager())
    {
        // When the profile changes externally (e.g., clean, import),
        // we must safely update our local copy 'slotsToEdit'.
        const auto& newSlots = getSharedSoundboardProfileManager().getCurrentSlots();

        // Copy the data element by element. This preserves the memory location of the
        // slots in 'slotsToEdit', keeping the references held by SoundboardSlotEditor valid.
        jassert(newSlots.size() == slotsToEdit.size());
        for (int i = 0; i < slotsToEdit.size(); ++i)
        {
            slotsToEdit.getReference(i) = newSlots.getReference(i);
        }

        // After safely updating the data, update the UI.
        updateAllSlotEditors();
    }
    else if (source == &LanguageManager::getInstance())
    {
        updateAllSlotEditors();
    }
}

void QuickKeySettingsContentComponent::chooseFileForSlot(int slotIndex)
{
    auto fc = std::make_shared<juce::FileChooser>("Select an audio file for Slot " + juce::String(slotIndex + 1),
        juce::File{},
        "*.mp3;*.wav;*.aif;*.aiff");

    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, fc, slotIndex](const juce::FileChooser& chooser)
        {
            if (chooser.getResults().isEmpty())
                return;

            slotsToEdit.getReference(slotIndex).audioFile = chooser.getResult();
            slotEditors[slotIndex]->updateLabels();
        });
}

void QuickKeySettingsContentComponent::setHotkeyForSlot(int slotIndex)
{
    auto& lang = LanguageManager::getInstance();
    auto& slot = slotsToEdit.getReference(slotIndex);

    class HotkeyCaptureComponent : public juce::Component, public juce::KeyListener
    {
    public:
        HotkeyCaptureComponent(SoundboardSlot& s, const juce::String& instructionText) : targetSlot(s)
        {
            setSize(350, 100);
            setWantsKeyboardFocus(true);
            addKeyListener(this);

            infoLabel.setText(instructionText, juce::dontSendNotification);
            infoLabel.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(infoLabel);
        }

        void resized() override { infoLabel.setBounds(getLocalBounds()); }

        bool keyPressed(const juce::KeyPress& key, juce::Component*) override
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            {
                int result = 0;
                if (!key.isKeyCurrentlyDown(juce::KeyPress::escapeKey))
                {
                    targetSlot.hotkey = key;
                    result = 1;
                }

                if (onFinished)
                    onFinished(result);

                dw->exitModalState(result);
            }
            return true;
        }

        std::function<void(int)> onFinished;

    private:
        SoundboardSlot& targetSlot;
        juce::Label infoLabel;
    };

    auto* captureComponent = new HotkeyCaptureComponent(slot, lang.get("quickKeySettingsWindow.hotkeyPromptMessage"));

    captureComponent->onFinished = [this, slotIndex](int modalResult)
        {
            if (modalResult != 0)
            {
                this->slotEditors[slotIndex]->updateLabels();
            }
        };

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
    auto profileDir = getSharedSoundboardProfileManager().getProfileDirectory();

    for (auto& slot : slotsToEdit)
    {
        // Chỉ xử lý các slot có file được gán và file đó nằm ngoài thư mục profile
        if (slot.audioFile.existsAsFile() && !slot.audioFile.isAChildOf(profileDir))
        {
            juce::File newFile = profileDir.getChildFile(slot.audioFile.getFileName());

            // FIX 1: Nếu file đích đã tồn tại, xóa nó đi để đảm bảo copy thành công
            if (newFile.existsAsFile())
            {
                if (!newFile.deleteFile())
                {
                    // Nếu không thể xóa file cũ, báo lỗi và bỏ qua
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "File Error",
                        "Could not replace existing file: " + newFile.getFileName());
                    continue; // Bỏ qua slot này
                }
            }

            // Thực hiện sao chép
            if (slot.audioFile.copyFileTo(newFile))
            {
                // QUAN TRỌNG: Cập nhật lại đối tượng File để nó trỏ đến file vừa được copy
                slot.audioFile = newFile;
            }
            else
            {
                // FIX 2: Nếu copy thất bại, xóa đường dẫn trong slot để tránh lưu rác
                slot.audioFile = juce::File{};
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "File Copy Error",
                    "Could not copy file: " + slot.audioFile.getFileName() + "\nThis sound will not be saved.");
            }
        }
    }

    // Bây giờ mới tiến hành lưu profile với các đường dẫn đã được cập nhật
    getSharedSoundboardProfileManager().saveProfile(slotsToEdit);

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Profile Saved",
        "Your soundboard profile has been saved successfully.");

    updateAllSlotEditors();
}

void QuickKeySettingsContentComponent::updateAllSlotEditors()
{
    auto& lang = LanguageManager::getInstance();
    saveButton.setButtonText(lang.get("quickKeySettingsWindow.saveProfile"));
    cleanButton.setButtonText(lang.get("quickKeySettingsWindow.cleanProfile"));
    importButton.setButtonText(lang.get("quickKeySettingsWindow.importProfile"));
    exportButton.setButtonText(lang.get("quickKeySettingsWindow.exportProfile"));

    for (auto* editor : slotEditors)
    {
        editor->updateLabels();
    }
}


//==============================================================================
// QuickKeySettingsWindow Implementation
//==============================================================================

// <<< MODIFIED: Constructor implementation >>>
QuickKeySettingsWindow::QuickKeySettingsWindow(std::function<void()> onWindowClosed)
    : DocumentWindow("Soundboard Settings", juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId), allButtons),
    onWindowClosedCallback(onWindowClosed)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new QuickKeySettingsContentComponent(), true);
    setResizable(true, true);
    setResizeLimits(700, 420, 900, 700);
    centreWithSize(700, 420);
    setVisible(true);
}

QuickKeySettingsWindow::~QuickKeySettingsWindow() {}

// <<< MODIFIED: closeButtonPressed calls the callback now >>>
void QuickKeySettingsWindow::closeButtonPressed()
{
    if (onWindowClosedCallback)
        onWindowClosedCallback();
    // The window is no longer responsible for deleting itself.
}