/*
  ==============================================================================

    PresetBarComponent.cpp
    (Fixed: Corrected function signatures and save logic)

  ==============================================================================
*/

#include "PresetBarComponent.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"
#include "../Windows/PresetManagerWindow.h"
#include "../../Data/AppState.h" 
#include "../../Data/PresetManager.h"

#include "../MainComponent/MainComponent.h"
#include "TrackComponent.h"
#include "MasterUtilityComponent.h"
#include "MenubarComponent.h"
#include "StatusBarComponent.h"


class PresetSlotComponent : public juce::Component, public juce::Button::Listener
{
public:
    PresetSlotComponent(PresetBarComponent& owner, int index) : ownerBar(owner), slotIndex(index)
    {
        addAndMakeVisible(loadButton);
        loadButton.addListener(this);
        addAndMakeVisible(assignButton);
        assignButton.setButtonText("+");
        assignButton.addListener(this);
    }
    void resized() override
    {
        auto bounds = getLocalBounds();
        assignButton.setBounds(bounds.removeFromRight(30));
        bounds.removeFromRight(2);
        loadButton.setBounds(bounds);
    }
    void buttonClicked(juce::Button* button) override
    {
        if (button == &loadButton)
            ownerBar.loadQuickPreset(slotIndex);
        else if (button == &assignButton)
            ownerBar.showAssignMenuForSlot(slotIndex);
    }
    void setButtonText(const juce::String& newText) { loadButton.setButtonText(newText); }
    juce::TextButton& getLoadButton() { return loadButton; }
private:
    PresetBarComponent& ownerBar;
    int slotIndex;
    juce::TextButton loadButton, assignButton;
};

PresetBarComponent::PresetBarComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedPresetManager().addChangeListener(this);
    AppState::getInstance().addChangeListener(this);
    addAndMakeVisible(presetRunningLabel);
    addAndMakeVisible(presetRunningValue);
    addAndMakeVisible(quickChoiceLabel);
    for (int i = 0; i < quickLoadSlots.size(); ++i)
    {
        quickLoadSlots[i] = std::make_unique<PresetSlotComponent>(*this, i);
        addAndMakeVisible(*quickLoadSlots[i]);
    }
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(managePresetsButton);
    presetRunningValue.setColour(juce::Label::textColourId, juce::Colour(0xffffbf00));
    presetRunningLabel.setFont(IdolUIHelpers::createRegularFont(14.0f));
    quickChoiceLabel.setFont(IdolUIHelpers::createBoldFont(14.0f));
    quickChoiceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    quickChoiceLabel.setJustificationType(juce::Justification::centredRight);
    savePresetButton.onClick = [this] { handleSaveAction(); };
    managePresetsButton.onClick = [] {
        if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
            app->showPresetManagerWindow();
        };
    updateTexts();
    updateQuickButtonLabels();
}

PresetBarComponent::~PresetBarComponent()
{
    AppState::getInstance().removeChangeListener(this);
    getSharedPresetManager().removeChangeListener(this);
    LanguageManager::getInstance().removeChangeListener(this);
}

void PresetBarComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

void PresetBarComponent::resized()
{
    const int mainComponentPadding = 5;
    const int parentWidth = getParentWidth();
    const int mainAreaWidth = parentWidth - (mainComponentPadding * 2);
    const int rightColumnWidth = mainAreaWidth / 3;
    const int verticalAlignmentLineX = parentWidth - mainComponentPadding - rightColumnWidth;
    auto bounds = getLocalBounds().reduced(10, 5);
    auto rightSection = bounds.removeFromRight(350);
    auto leftSection = bounds.removeFromLeft(250);
    managePresetsButton.setBounds(rightSection.removeFromRight(160));
    rightSection.removeFromRight(5);
    savePresetButton.setBounds(rightSection);
    presetRunningLabel.setBounds(leftSection.removeFromLeft(120));
    presetRunningValue.setBounds(leftSection);
    auto quickSlotsArea = bounds;
    quickSlotsArea.setRight(this->getLocalPoint(nullptr, juce::Point<int>{ verticalAlignmentLineX, 0 }).getX());
    quickChoiceLabel.setBounds(quickSlotsArea.removeFromLeft(100));
    quickSlotsArea.removeFromLeft(10);
    const int buttonWidth = 140;
    const int buttonGap = 4;
    for (const auto& slot : quickLoadSlots)
    {
        slot->setBounds(quickSlotsArea.removeFromLeft(buttonWidth));
        quickSlotsArea.removeFromLeft(buttonGap);
    }
}

void PresetBarComponent::loadPresetTask(const juce::String& presetName)
{
    if (presetName.isEmpty()) return;

    auto* mainComp = findParentComponentOfClass<MainComponent>();
    if (mainComp == nullptr) return;

    auto* statusBar = mainComp->getStatusBarComponent();
    if (statusBar == nullptr) return;

    AppState::getInstance().setIsLoadingPreset(true);

    auto presetFile = getSharedPresetManager().getPresetDirectory().getChildFile(presetName + ".xml");
    if (!presetFile.existsAsFile())
    {
        statusBar->setStatusMessage("Error: Preset file not found!", true);
        AppState::getInstance().setIsLoadingPreset(false);
        return;
    }

    auto xmlDoc = juce::parseXML(presetFile);
    if (xmlDoc == nullptr || !xmlDoc->hasTagName("Preset"))
    {
        statusBar->setStatusMessage("Error: Invalid preset file!", true);
        AppState::getInstance().setIsLoadingPreset(false);
        return;
    }
    juce::ValueTree newState = juce::ValueTree::fromXml(*xmlDoc);

    // Trích xuất thông tin trạng thái khóa từ preset
    const bool isLocked = newState.getProperty(Identifiers::lockState, false);
    const juce::String passwordHash = newState.getProperty(Identifiers::lockPasswordHash, "");

    bool chainsAreIdentical = audioEngine.getVocalProcessor().isPluginChainIdentical(newState)
        && audioEngine.getMusicProcessor().isPluginChainIdentical(newState)
        && audioEngine.getMasterProcessor().isPluginChainIdentical(newState);
    // (Giả sử các FX chain cũng được kiểm tra trong một bản triển khai đầy đủ)

    if (chainsAreIdentical)
    {
        statusBar->setStatusMessage("Hot-swapping preset...", false);
        if (audioEngine.tryHotSwapState(newState))
        {
            AppState::getInstance().markAsSaved(presetName);

            // Áp dụng trạng thái khóa sau khi hot-swap thành công
            AppState::getInstance().loadLockState(isLocked, passwordHash);

            statusBar->setStatusMessage("Load successful!", false);
            juce::Timer::callAfterDelay(2000, [statusBar]() { if (statusBar) statusBar->setStatusMessage("", false); });
        }
        else
        {
            statusBar->setStatusMessage("Hot-swap failed, performing full reload...", true);
            performFullReload(newState, presetName, isLocked, passwordHash);
        }
    }
    else
    {
        statusBar->setStatusMessage("Plugin chain changed, performing full reload...", false);
        performFullReload(newState, presetName, isLocked, passwordHash);
    }

    juce::Timer::callAfterDelay(100, [] { AppState::getInstance().setIsLoadingPreset(false); });
}

void PresetBarComponent::performFullReload(const juce::ValueTree& newState, const juce::String& presetName, bool isLocked, const juce::String& passwordHash)
{
    auto* mainComp = findParentComponentOfClass<MainComponent>();
    auto* statusBar = mainComp ? mainComp->getStatusBarComponent() : nullptr;
    if (mainComp == nullptr || statusBar == nullptr) return;

    bool preparationSuccess = audioEngine.prepareToLoadState(newState);

    if (!preparationSuccess)
    {
        statusBar->setStatusMessage("Critical Error: Failed to prepare plugins for loading!", true);
        return;
    }

    auto& deviceManager = mainComp->getAudioDeviceManager();
    const juce::String savedVocalInputName = audioEngine.getVocalInputChannelName();
    const juce::String savedMusicInputName = audioEngine.getMusicInputChannelName();
    const juce::String savedOutputName = audioEngine.getSelectedOutputChannelPairName();

    deviceManager.closeAudioDevice();

    mainComp->getVocalTrack().closeAllPluginWindows();
    mainComp->getMusicTrack().closeAllPluginWindows();
    mainComp->getMasterUtilityComponent().closeAllPluginWindows();

    audioEngine.commitStateLoad();
    AppState::getInstance().markAsSaved(presetName);

    // Áp dụng trạng thái khóa sau khi đã nạp thành công
    AppState::getInstance().loadLockState(isLocked, passwordHash);

    deviceManager.restartLastAudioDevice();

    juce::Timer::callAfterDelay(500, [this, mainComp, savedVocalInputName, savedMusicInputName, savedOutputName]()
        {
            if (mainComp == nullptr) return;
            audioEngine.setVocalInputChannelByName(savedVocalInputName);
            audioEngine.setMusicInputChannelByName(savedMusicInputName);
            audioEngine.setSelectedOutputChannelsByName(savedOutputName);
            if (auto* vocalSelector = mainComp->getVocalTrack().getChannelSelector())
                vocalSelector->setSelectedChannelByName(savedVocalInputName);
            if (auto* musicSelector = mainComp->getMusicTrack().getChannelSelector())
                musicSelector->setSelectedChannelByName(savedMusicInputName);
            if (auto* menubar = mainComp->getMenubarComponent())
                if (auto* outputSelector = menubar->getOutputSelector())
                    outputSelector->setSelectedChannelByName(savedOutputName);
        });
}

void PresetBarComponent::showAssignMenuForSlot(int slotIndex)
{
    juce::PopupMenu menu;
    auto& lang = LanguageManager::getInstance();
    juce::PopupMenu assignMenu;
    auto presetDir = getSharedPresetManager().getPresetDirectory();
    auto presetFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*.xml");
    int presetItemId = 1;
    for (const auto& file : presetFiles)
    {
        assignMenu.addItem(presetItemId++, file.getFileNameWithoutExtension());
    }
    menu.addSubMenu(lang.get("presetbar.assignPreset"), assignMenu);
    menu.addSeparator();
    menu.addItem(presetFiles.size() + 1, lang.get("presetbar.clearSlot"));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&quickLoadSlots[slotIndex]->getLoadButton()),
        [this, slotIndex, presetFiles](int result)
        {
            if (result == 0) return;
            auto& appState = AppState::getInstance();
            if (result > 0 && result <= presetFiles.size())
            {
                appState.assignQuickPreset(slotIndex, presetFiles[result - 1].getFileNameWithoutExtension());
            }
            else if (result > 0)
            {
                appState.assignQuickPreset(slotIndex, {});
            }
        });
}

// <<< FIXED: Uses audioEngine.getFullState() for consistent saving >>>
void PresetBarComponent::saveAsNewPreset(std::function<void(bool)> onComplete)
{
    auto& lang = LanguageManager::getInstance();
    auto fc = std::make_shared<juce::FileChooser>(lang.get("presetbar.saveAsTitle"), getSharedPresetManager().getPresetDirectory(), "*.xml");
    fc->launchAsync(juce::FileBrowserComponent::saveMode,
        [this, fc, onComplete](const juce::FileChooser& chooser)
        {
            auto resultFile = chooser.getResult();
            if (resultFile != juce::File{})
            {
                auto currentState = audioEngine.getFullState();
                if (auto xml = std::unique_ptr<juce::XmlElement>(currentState.createXml()))
                {
                    xml->writeTo(resultFile, {});
                    auto newPresetName = resultFile.getFileNameWithoutExtension();
                    AppState::getInstance().markAsSaved(newPresetName);
                    if (onComplete) onComplete(true);
                }
            }
            else
            {
                if (onComplete) onComplete(false);
            }
        });
}

// <<< FIXED: Uses audioEngine.getFullState() for consistent saving >>>
void PresetBarComponent::handleSaveAction()
{
    auto& lang = LanguageManager::getInstance();
    auto currentPresetName = AppState::getInstance().getCurrentPresetName();
    if (currentPresetName != lang.get("presetbar.noPresetLoaded") && currentPresetName.isNotEmpty())
    {
        auto* alert = new juce::AlertWindow(lang.get("presetbar.saveTitle"), lang.get("presetbar.saveMessage"), juce::AlertWindow::QuestionIcon);
        alert->addButton(lang.get("presetbar.saveOverwrite"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton(lang.get("presetbar.saveAsNew"), 2);
        alert->addButton(lang.get("alerts.cancelButton"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, currentPresetName](int result)
            {
                if (result == 1)
                {
                    auto presetFileToOverwrite = getSharedPresetManager().getPresetDirectory().getChildFile(currentPresetName + ".xml");
                    auto currentState = audioEngine.getFullState();
                    if (auto xml = std::unique_ptr<juce::XmlElement>(currentState.createXml()))
                    {
                        xml->writeTo(presetFileToOverwrite, {});
                        AppState::getInstance().markAsSaved(currentPresetName);
                    }
                }
                else if (result == 2)
                {
                    saveAsNewPreset(nullptr);
                }
            }
        ), true);
    }
    else
    {
        saveAsNewPreset(nullptr);
    }
}

void PresetBarComponent::loadQuickPreset(int slotIndex)
{
    auto presetName = AppState::getInstance().getQuickPresetName(slotIndex);
    if (presetName.isEmpty()) return;

    if (AppState::getInstance().isPresetDirty())
    {
        auto& lang = LanguageManager::getInstance();
        juce::StringArray buttons;
        buttons.add(lang.get("alerts.saveButton"));
        buttons.add(lang.get("alerts.dontSaveButton"));
        buttons.add(lang.get("alerts.cancelButton"));
        auto* alert = new juce::AlertWindow(lang.get("alerts.saveChangesTitle"), lang.get("alerts.saveChangesMessage"), juce::AlertWindow::QuestionIcon);
        alert->addTextBlock(AppState::getInstance().getCurrentPresetName());
        for (const auto& button : buttons)
            alert->addButton(button, buttons.indexOf(button) + 1);
        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, presetName](int result)
            {
                if (result == 1)
                {
                    saveAsNewPreset([this, presetName](bool wasSaved) { if (wasSaved) loadPresetTask(presetName); });
                }
                else if (result == 2)
                {
                    loadPresetTask(presetName);
                }
            }), true);
    }
    else
    {
        loadPresetTask(presetName);
    }
}

void PresetBarComponent::loadPresetByName(const juce::String& name)
{
    loadPresetTask(name);
}

void PresetBarComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    presetRunningLabel.setText(lang.get("presetbar.presetRunning"), juce::dontSendNotification);
    quickChoiceLabel.setText(lang.get("presetbar.quickChoice"), juce::dontSendNotification);
    savePresetButton.setButtonText(lang.get("managers.savePreset"));
    managePresetsButton.setButtonText(lang.get("managers.managePresets"));
    auto currentPresetName = AppState::getInstance().getCurrentPresetName();
    presetRunningValue.setText(currentPresetName, juce::dontSendNotification);
    auto presetFile = getSharedPresetManager().getPresetDirectory().getChildFile(currentPresetName + ".xml");
    if (currentPresetName.isEmpty() || !presetFile.existsAsFile())
    {
        presetRunningValue.setText(lang.get("presetbar.noPresetLoaded"), juce::dontSendNotification);
    }
    updateQuickButtonLabels();
    repaint();
}

void PresetBarComponent::updateQuickButtonLabels()
{
    auto& appState = AppState::getInstance();
    auto& lang = LanguageManager::getInstance();
    for (int i = 0; i < quickLoadSlots.size(); ++i)
    {
        auto presetName = appState.getQuickPresetName(i);
        if (presetName.isNotEmpty())
        {
            quickLoadSlots[i]->setButtonText(presetName);
        }
        else
        {
            quickLoadSlots[i]->setButtonText(lang.get("presetbar.emptySlot") + " " + juce::String(i + 1));
        }
    }
}

void PresetBarComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &getSharedPresetManager()) { /* No longer needed */ }
    else if (source == &LanguageManager::getInstance() || source == &AppState::getInstance())
    {
        updateTexts();
    }
}