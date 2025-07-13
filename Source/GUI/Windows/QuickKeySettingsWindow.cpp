#include "QuickKeySettingsWindow.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"

//==============================================================================
// CÁC HÀM TRỢ GIÚP DỰA TRÊN Ý TƯỞNG CỦA BẠN
//==============================================================================

// Bước 1: Hàm dịch mã VK_ của Windows thành tên phím đọc được
static juce::String getReadableKeyNameFromVK(int vkCode)
{
    switch (vkCode)
    {
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return "Ctrl";
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:       return "Shift";
    case VK_MENU: case VK_LMENU: case VK_RMENU:          return "Alt";
    case VK_LWIN: case VK_RWIN:                          return "Win";

    case VK_RETURN: return "Enter";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE:  return "Space";
    case VK_BACK:   return "Backspace";
    case VK_TAB:    return "Tab";
    case VK_DELETE: return "Delete";

    case VK_LEFT:   return "Left";
    case VK_RIGHT:  return "Right";
    case VK_UP:     return "Up";
    case VK_DOWN:   return "Down";

    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";

    default:
        if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9'))
            return juce::String::charToString((juce_wchar)vkCode);
    }

    return "[?]";
}

// Bước 2: Hàm định dạng KeyPress hoàn chỉnh
static juce::String formatVKHotkeyText(const juce::KeyPress& key)
{
    if (!key.isValid())
        return {};

    juce::StringArray parts;
    const auto mods = key.getModifiers();

    if (mods.isCtrlDown())    parts.add("Ctrl");
    if (mods.isShiftDown())   parts.add("Shift");
    if (mods.isAltDown())     parts.add("Alt");

    // trên Windows, Command key được map thành phím Windows
    if (mods.isCommandDown()) parts.add("Win");

    const int vkCode = key.getKeyCode();
    juce::String mainKey = getReadableKeyNameFromVK(vkCode);

    // Chỉ thêm tên phím chính nếu nó chưa có trong danh sách modifier
    // Ví dụ: khi nhấn Ctrl, mainKey là "Ctrl", đã có trong parts, không thêm nữa.
    // Khi nhấn Ctrl + S, mainKey là "S", không có trong parts, sẽ được thêm vào.
    if (!parts.contains(mainKey))
        parts.add(mainKey);

    return parts.joinIntoString(" + ");
}


//==============================================================================
// QuickKeySettingsContentComponent Implementation
//==============================================================================

QuickKeySettingsContentComponent::QuickKeySettingsContentComponent()
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedSoundboardProfileManager().addChangeListener(this);

    slotsToEdit = getSharedSoundboardProfileManager().getCurrentSlots();

    for (int i = 0; i < 9; ++i)
    {
        auto* button = slotGridButtons.add(new juce::TextButton(juce::String(i + 1)));
        button->setRadioGroupId(1);
        button->setClickingTogglesState(true);
        const int index = i;
        button->onClick = [this, index] {
            cancelCaptureMode();
            showDetailsForSlot(index);
            slotGridButtons[index]->setToggleState(true, juce::sendNotification);
            };
        addAndMakeVisible(button);
    }

    addAndMakeVisible(nameLabel);
    addAndMakeVisible(nameEditor);
    nameEditor.addListener(this);

    addAndMakeVisible(assignFileButton);
    assignFileButton.onClick = [this] { chooseFileForSlot(selectedSlot); };

    addAndMakeVisible(setHotkeyButton);
    setHotkeyButton.onClick = [this] { setHotkeyForSlot(selectedSlot); };

    addAndMakeVisible(saveButton);
    saveButton.onClick = [this] { saveChanges(); };

    addAndMakeVisible(cleanButton);
    cleanButton.onClick = [this] {
        auto& lang = LanguageManager::getInstance();
        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            lang.get("alerts.cleanProfileConfirmTitle"), lang.get("alerts.cleanProfileConfirmMessage"),
            lang.get("quickKeySettingsWindow.yes"), lang.get("quickKeySettingsWindow.no"), this,
            juce::ModalCallbackFunction::create([this](int result) {
                if (result == 1) getSharedSoundboardProfileManager().cleanProfile();
                }));
        };

    addAndMakeVisible(importButton);
    importButton.onClick = [this] {
        auto fc = std::make_shared<juce::FileChooser>("Import Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, fc](const juce::FileChooser& chooser) {
                if (!chooser.getResults().isEmpty())
                    getSharedSoundboardProfileManager().importProfile(chooser.getResult());
            });
        };

    addAndMakeVisible(exportButton);
    exportButton.onClick = [this] {
        auto fc = std::make_shared<juce::FileChooser>("Export Soundboard Profile", juce::File{}, "*.zip");
        fc->launchAsync(juce::FileBrowserComponent::saveMode,
            [this, fc](const juce::FileChooser& chooser) {
                if (!chooser.getResults().isEmpty())
                    getSharedSoundboardProfileManager().exportProfile(chooser.getResult());
            });
        };

    startTimerHz(10);
    updateAllUITexts();
    showDetailsForSlot(0);
    slotGridButtons[0]->setToggleState(true, juce::sendNotification);
}

QuickKeySettingsContentComponent::~QuickKeySettingsContentComponent()
{
    stopTimer();
    cancelCaptureMode();
    LanguageManager::getInstance().removeChangeListener(this);
    getSharedSoundboardProfileManager().removeChangeListener(this);
}

void QuickKeySettingsContentComponent::timerCallback()
{
    if (isCapturingHotkey)
    {
        if (auto* topLevelComp = getTopLevelComponent())
            if (auto* topLevelWin = dynamic_cast<juce::TopLevelWindow*>(topLevelComp))
                if (!topLevelWin->isActiveWindow())
                    cancelCaptureMode();
    }
}

void QuickKeySettingsContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(editorGroupBounds.toFloat(), 5.0f, 1.0f);
    g.drawRoundedRectangle(profileGroupBounds.toFloat(), 5.0f, 1.0f);
}

void QuickKeySettingsContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto leftPanel = bounds.removeFromLeft(240);
    bounds.removeFromLeft(10);
    auto rightPanel = bounds;

    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    grid.templateRows = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
    grid.setGap(juce::Grid::Px(5));

    for (auto* button : slotGridButtons)
        grid.items.add(juce::GridItem(button));

    grid.performLayout(leftPanel);

    profileGroupBounds = rightPanel.removeFromBottom(130);
    rightPanel.removeFromBottom(10);
    editorGroupBounds = rightPanel;

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
    juce::ignoreUnused(button);
}

void QuickKeySettingsContentComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &nameEditor)
    {
        slotsToEdit.getReference(selectedSlot).displayName = editor.getText();
    }
}

void QuickKeySettingsContentComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
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
    selectedSlot = slotIndex;
    for (int i = 0; i < slotGridButtons.size(); ++i)
        if (slotGridButtons[i]->getToggleState() && i != selectedSlot)
            slotGridButtons[i]->setToggleState(false, juce::dontSendNotification);

    auto& lang = LanguageManager::getInstance();
    auto& slot = slotsToEdit.getReference(selectedSlot);
    nameEditor.setText(slot.displayName, juce::dontSendNotification);
    assignFileButton.setButtonText(slot.isEmpty() ? lang.get("quickKeySettingsWindow.noFile") : slot.audioFile.getFileName());

    // Bước 3: Áp dụng hàm mới để hiển thị tên hotkey
    juce::String hotkeyText;
    if (slot.hotkey.isValid())
        hotkeyText = formatVKHotkeyText(slot.hotkey);

    if (hotkeyText.isEmpty())
        hotkeyText = lang.get("quickKeySettingsWindow.noHotkey");

    setHotkeyButton.setButtonText(hotkeyText);
    setHotkeyButton.setColour(juce::TextButton::buttonColourId, getLookAndFeel().findColour(juce::TextButton::buttonColourId));
}

void QuickKeySettingsContentComponent::chooseFileForSlot(int slotIndex)
{
    auto fc = std::make_shared<juce::FileChooser>("Select an audio file for Slot " + juce::String(slotIndex + 1),
        juce::File{}, "*.mp3;*.wav;*.aif;*.aiff");
    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, fc, slotIndex](const juce::FileChooser& chooser) {
            if (chooser.getResults().isEmpty()) return;
            slotsToEdit.getReference(slotIndex).audioFile = chooser.getResult();
            showDetailsForSlot(slotIndex);
        });
}

void QuickKeySettingsContentComponent::setHotkeyForSlot(int slotIndex)
{
    if (isCapturingHotkey)
    {
        cancelCaptureMode();
        return;
    }

    if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
    {
        if (auto* hotkeyManager = app->getPublicGlobalHotkeyManager())
        {
            isCapturingHotkey = true;
            setHotkeyButton.setButtonText(LanguageManager::getInstance().get("quickKeySettingsWindow.hotkeyPromptMessage"));
            setHotkeyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);

            hotkeyManager->setCaptureMode([this, slotIndex](const juce::KeyPress& key) {
                slotsToEdit.getReference(slotIndex).hotkey = key;
                isCapturingHotkey = false;
                showDetailsForSlot(slotIndex);
                });
        }
    }
}

void QuickKeySettingsContentComponent::saveChanges()
{
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
    auto& lang = LanguageManager::getInstance();
    nameLabel.setText(lang.get("quickKeySettingsWindow.slotNameLabel"), juce::dontSendNotification);
    saveButton.setButtonText(lang.get("quickKeySettingsWindow.saveProfile"));
    cleanButton.setButtonText(lang.get("quickKeySettingsWindow.cleanProfile"));
    importButton.setButtonText(lang.get("quickKeySettingsWindow.importProfile"));
    exportButton.setButtonText(lang.get("quickKeySettingsWindow.exportProfile"));
}

void QuickKeySettingsContentComponent::cancelCaptureMode()
{
    if (isCapturingHotkey)
    {
        isCapturingHotkey = false;
        if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
            if (auto* manager = app->getPublicGlobalHotkeyManager())
                manager->setCaptureMode(nullptr);
        showDetailsForSlot(selectedSlot);
    }
}

//==============================================================================
// QuickKeySettingsWindow Implementation
//==============================================================================
QuickKeySettingsWindow::QuickKeySettingsWindow(std::function<void()> onWindowClosed)
    : DocumentWindow("Soundboard Settings", juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId), allButtons),
    onWindowClosedCallback(onWindowClosed)
{
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
    if (onWindowClosedCallback)
        onWindowClosedCallback();
}