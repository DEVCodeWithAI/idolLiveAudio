/*
  ==============================================================================

    PresetBarComponent.cpp
    (Layout refined to align with main columns)

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


// ==============================================================================
// A new inner class to group a slot button and its assign button
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

    void setButtonText(const juce::String& newText)
    {
        loadButton.setButtonText(newText);
    }

    juce::TextButton& getLoadButton() { return loadButton; }

private:
    PresetBarComponent& ownerBar;
    int slotIndex;
    juce::TextButton loadButton, assignButton;
};
// ==============================================================================


PresetBarComponent::PresetBarComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    // Constructor logic remains the same
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
    // Destructor logic remains the same
    AppState::getInstance().removeChangeListener(this);
    getSharedPresetManager().removeChangeListener(this);
    LanguageManager::getInstance().removeChangeListener(this);
}

void PresetBarComponent::paint(juce::Graphics& g)
{
    // Paint logic remains the same
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

void PresetBarComponent::resized()
{
    // --- 1. Calculate the vertical alignment line's X coordinate ---
    // This logic must mirror the layout calculation in MainComponent.cpp
    const int mainComponentPadding = 5;
    const int parentWidth = getParentWidth();
    const int mainAreaWidth = parentWidth - (mainComponentPadding * 2);
    const int rightColumnWidth = mainAreaWidth / 3;
    // This is the X coordinate of the dividing line between the left and right columns
    const int verticalAlignmentLineX = parentWidth - mainComponentPadding - rightColumnWidth;

    // --- 2. Layout the component ---
    auto bounds = getLocalBounds().reduced(10, 5);

    // Layout the static left and right sections first
    auto rightSection = bounds.removeFromRight(350);
    auto leftSection = bounds.removeFromLeft(250);

    managePresetsButton.setBounds(rightSection.removeFromRight(160));
    rightSection.removeFromRight(5);
    savePresetButton.setBounds(rightSection);

    presetRunningLabel.setBounds(leftSection.removeFromLeft(120));
    presetRunningValue.setBounds(leftSection);

    // --- 3. Define the area for the quick slots using the calculated alignment line ---
    auto quickSlotsArea = bounds;

    // <<< FIX IS HERE: Convert parent's X coordinate to local and remove incorrect padding subtraction >>>
    quickSlotsArea.setRight(this->getLocalPoint(nullptr, juce::Point<int>{ verticalAlignmentLineX, 0 }).getX());

    // --- 4. Layout the label and the 5 slots inside this newly defined area ---
    quickChoiceLabel.setBounds(quickSlotsArea.removeFromLeft(100));
    quickSlotsArea.removeFromLeft(10);

    // <<< CHANGE: Increased button width from 120 to 130 >>>
    const int buttonWidth = 140;
    const int buttonGap = 4;
    for (const auto& slot : quickLoadSlots)
    {
        slot->setBounds(quickSlotsArea.removeFromLeft(buttonWidth));
        quickSlotsArea.removeFromLeft(buttonGap);
    }
}

void PresetBarComponent::showAssignMenuForSlot(int slotIndex)
{
    // Logic remains the same
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
                auto presetName = presetFiles[result - 1].getFileNameWithoutExtension();
                appState.assignQuickPreset(slotIndex, presetName);
            }
            else if (result > 0)
            {
                appState.assignQuickPreset(slotIndex, {});
            }
        });
}

void PresetBarComponent::saveAsNewPreset(std::function<void(bool)> onComplete)
{
    // Logic remains the same
    auto& presetManager = getSharedPresetManager();
    auto& lang = LanguageManager::getInstance();
    auto fc = std::make_shared<juce::FileChooser>(lang.get("presetbar.saveAsTitle"),
        presetManager.getPresetDirectory(),
        "*.xml");
    fc->launchAsync(juce::FileBrowserComponent::saveMode,
        [this, &presetManager, fc, onComplete](const juce::FileChooser& chooser)
        {
            auto resultFile = chooser.getResult();
            if (resultFile != juce::File{})
            {
                presetManager.savePreset(audioEngine, resultFile);
                auto newPresetName = resultFile.getFileNameWithoutExtension();
                AppState::getInstance().markAsSaved(newPresetName);
                if (onComplete) onComplete(true);
            }
            else
            {
                if (onComplete) onComplete(false);
            }
        });
}

void PresetBarComponent::handleSaveAction()
{
    // Logic remains the same
    auto& lang = LanguageManager::getInstance();
    auto currentPresetName = AppState::getInstance().getCurrentPresetName();
    if (currentPresetName != lang.get("presetbar.noPresetLoaded") && currentPresetName.isNotEmpty())
    {
        auto* alert = new juce::AlertWindow(lang.get("presetbar.saveTitle"),
            lang.get("presetbar.saveMessage"),
            juce::AlertWindow::QuestionIcon);
        alert->addButton(lang.get("presetbar.saveOverwrite"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton(lang.get("presetbar.saveAsNew"), 2);
        alert->addButton(lang.get("alerts.cancelButton"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
        alert->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, currentPresetName](int result)
            {
                if (result == 1)
                {
                    auto& presetManager = getSharedPresetManager();
                    auto presetFileToOverwrite = presetManager.getPresetDirectory().getChildFile(currentPresetName + ".xml");
                    presetManager.savePreset(audioEngine, presetFileToOverwrite);
                    AppState::getInstance().markAsSaved(currentPresetName);
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
    // Logic remains the same
    auto presetName = AppState::getInstance().getQuickPresetName(slotIndex);
    if (presetName.isEmpty())
        return;

    if (AppState::getInstance().isPresetDirty())
    {
        auto& lang = LanguageManager::getInstance();
        juce::StringArray buttons;
        buttons.add(lang.get("alerts.saveButton"));
        buttons.add(lang.get("alerts.dontSaveButton"));
        buttons.add(lang.get("alerts.cancelButton"));
        auto* alert = new juce::AlertWindow(lang.get("alerts.saveChangesTitle"),
            lang.get("alerts.saveChangesMessage"),
            juce::AlertWindow::QuestionIcon);
        alert->addTextBlock(AppState::getInstance().getCurrentPresetName());
        for (const auto& button : buttons)
            alert->addButton(button, buttons.indexOf(button) + 1);
        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, presetName](int result)
            {
                if (result == 1)
                {
                    saveAsNewPreset([this, presetName](bool wasSaved) {
                        if (wasSaved)
                            performLoadTask(presetName);
                        });
                }
                else if (result == 2)
                {
                    performLoadTask(presetName);
                }
            }), true);
    }
    else
    {
        performLoadTask(presetName);
    }
}

void PresetBarComponent::performLoadTask(const juce::String& presetName)
{
    // Logic remains the same
    if (presetName.isEmpty()) return;

    if (auto* mainComp = findParentComponentOfClass<MainComponent>())
    {
        mainComp->getVocalTrack().closeAllPluginWindows();
        mainComp->getMusicTrack().closeAllPluginWindows();
        mainComp->getMasterUtilityComponent().closeAllPluginWindows();

        auto& presetManager = getSharedPresetManager();
        auto presetFile = presetManager.getPresetDirectory().getChildFile(presetName + ".xml");

        if (presetFile.existsAsFile())
        {
            presetManager.loadPreset(audioEngine, presetFile);
            AppState::getInstance().markAsSaved(presetName);
        }
    }
}

void PresetBarComponent::loadPresetByName(const juce::String& name)
{
    // Logic remains the same
    performLoadTask(name);
}

void PresetBarComponent::updateTexts()
{
    // Logic remains the same
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
    // Logic remains the same
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
    // Logic remains the same
    if (source == &getSharedPresetManager())
    {
    }
    else if (source == &LanguageManager::getInstance() || source == &AppState::getInstance())
    {
        updateTexts();
    }
}