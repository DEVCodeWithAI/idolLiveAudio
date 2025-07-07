#include "PresetBarComponent.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"
#include "../Windows/PresetManagerWindow.h"
#include "../../Data/AppState.h" 

// <<< FIX: Đường dẫn include chính xác đến MainComponent.h >>>
#include "../MainComponent/MainComponent.h"
#include "TrackComponent.h"
#include "MasterUtilityComponent.h"

PresetBarComponent::PresetBarComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedPresetManager().addChangeListener(this);
    AppState::getInstance().addChangeListener(this);

    addAndMakeVisible(presetRunningLabel);
    addAndMakeVisible(presetRunningValue);
    addAndMakeVisible(quickChoiceLabel);
    addAndMakeVisible(quickChoiceBox);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(savePresetButton);
    addAndMakeVisible(managePresetsButton);

    presetRunningValue.setColour(juce::Label::textColourId, juce::Colour(0xffffbf00));

    presetRunningLabel.setFont(IdolUIHelpers::createRegularFont(14.0f));
    quickChoiceLabel.setFont(IdolUIHelpers::createRegularFont(14.0f));

    savePresetButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPresetFromButton(); };

    managePresetsButton.onClick = [] {
        if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
            app->showPresetManagerWindow();
        };

    updateTexts();
    populatePresetBox();
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

// <<< SỬA: Viết lại hoàn toàn hàm resized() để có layout mới >>>
void PresetBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);
    const int padding = 10;

    // 1. Chia toàn bộ thanh thành 2 phần: Trái (2/3) và Phải (1/3)
    auto rightSection = bounds.removeFromRight(bounds.getWidth() / 3);
    bounds.removeFromRight(padding);
    auto leftSection = bounds;

    // 2. Layout cho khu vực bên phải (Save, Manage)
    {
        managePresetsButton.setBounds(rightSection.removeFromRight(160));
        rightSection.removeFromRight(5);
        savePresetButton.setBounds(rightSection);
    }

    // 3. Layout cho khu vực bên trái
    {
        // Nút Load sẽ nằm ở ganz rìa phải của khu vực này
        loadButton.setBounds(leftSection.removeFromRight(80));
        leftSection.removeFromRight(padding);

        // Các thành phần còn lại được layout trong không gian còn lại
        quickChoiceBox.setBounds(leftSection.removeFromRight(180));
        leftSection.removeFromRight(padding);
        quickChoiceLabel.setBounds(leftSection.removeFromRight(100));
        leftSection.removeFromRight(padding);

        presetRunningLabel.setBounds(leftSection.removeFromLeft(120));
        presetRunningValue.setBounds(leftSection);
    }
}

void PresetBarComponent::populatePresetBox()
{
    auto lastSelectedText = quickChoiceBox.getText();
    quickChoiceBox.clear(juce::dontSendNotification);

    auto presetDir = getSharedPresetManager().getPresetDirectory();
    auto presetFiles = presetDir.findChildFiles(juce::File::findFiles, false, "*.xml");

    int id = 1;
    for (const auto& file : presetFiles)
    {
        quickChoiceBox.addItem(file.getFileNameWithoutExtension(), id++);
    }

    quickChoiceBox.setText(lastSelectedText, juce::dontSendNotification);
}

void PresetBarComponent::saveAsNewPreset(std::function<void(bool)> onComplete)
{
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
                populatePresetBox();
                auto newPresetName = resultFile.getFileNameWithoutExtension();

                AppState::getInstance().markAsSaved(newPresetName);

                quickChoiceBox.setText(newPresetName, juce::dontSendNotification);
                if (onComplete) onComplete(true);
            }
            else
            {
                if (onComplete) onComplete(false);
            }
        });
}


void PresetBarComponent::savePreset()
{
    auto& presetManager = getSharedPresetManager();
    auto& lang = LanguageManager::getInstance();

    auto currentPresetName = AppState::getInstance().getCurrentPresetName();

    if (currentPresetName != lang.get("presetbar.noPresetLoaded") && currentPresetName.isNotEmpty())
    {
        auto presetFileToOverwrite = presetManager.getPresetDirectory().getChildFile(currentPresetName + ".xml");

        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            lang.get("presetbar.overwriteTitle"),
            lang.get("presetbar.overwriteMessage").replace("{{presetName}}", currentPresetName),
            lang.get("presetbar.yes"),
            lang.get("presetbar.no"),
            nullptr,
            juce::ModalCallbackFunction::create([this, &presetManager, presetFileToOverwrite, currentPresetName](int result)
                {
                    if (result == 1) // OK pressed
                    {
                        presetManager.savePreset(audioEngine, presetFileToOverwrite);
                        AppState::getInstance().markAsSaved(currentPresetName); // Reset dirty flag after saving
                    }
                }));
    }
    else
    {
        saveAsNewPreset(nullptr); // Save as new if no preset is loaded
    }
}

void PresetBarComponent::loadPresetFromButton()
{
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

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this](int result)
            {
                if (result == 1) // Save As...
                {
                    saveAsNewPreset([this](bool wasSaved) {
                        if (wasSaved)
                        {
                            performLoadTask();
                        }
                        });
                }
                else if (result == 2) // Don't Save
                {
                    performLoadTask();
                }
            }), true);
    }
    else
    {
        performLoadTask();
    }
}

void PresetBarComponent::performLoadTask()
{
    auto selectedId = quickChoiceBox.getSelectedId();
    if (selectedId == 0) return;

    if (auto* mainComp = findParentComponentOfClass<MainComponent>())
    {
        mainComp->getVocalTrack().closeAllPluginWindows();
        mainComp->getMusicTrack().closeAllPluginWindows();
        mainComp->getMasterUtilityComponent().closeAllPluginWindows();

        auto presetName = quickChoiceBox.getText();
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
    for (int i = 1; i <= quickChoiceBox.getNumItems(); ++i)
    {
        if (quickChoiceBox.getItemText(i - 1) == name)
        {
            quickChoiceBox.setSelectedId(i, juce::dontSendNotification);
            performLoadTask();
            return;
        }
    }
    DBG("Preset not found in ComboBox: " + name);
}

void PresetBarComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    presetRunningLabel.setText(lang.get("presetbar.presetRunning"), juce::dontSendNotification);
    quickChoiceLabel.setText(lang.get("presetbar.quickChoice"), juce::dontSendNotification);
    loadButton.setButtonText(lang.get("presetbar.load"));
    savePresetButton.setButtonText(lang.get("managers.savePreset"));
    managePresetsButton.setButtonText(lang.get("managers.managePresets"));

    auto currentPresetName = AppState::getInstance().getCurrentPresetName();
    presetRunningValue.setText(currentPresetName, juce::dontSendNotification);

    auto presetFile = getSharedPresetManager().getPresetDirectory().getChildFile(currentPresetName + ".xml");
    if (currentPresetName.isEmpty() || !presetFile.existsAsFile())
    {
        presetRunningValue.setText(lang.get("presetbar.noPresetLoaded"), juce::dontSendNotification);
    }

    repaint();
}

void PresetBarComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &getSharedPresetManager())
    {
        populatePresetBox();
    }
    else if (source == &LanguageManager::getInstance() || source == &AppState::getInstance())
    {
        updateTexts();
    }
}