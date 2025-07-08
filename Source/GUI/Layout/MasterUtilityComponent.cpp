/*
  ==============================================================================

    MasterUtilityComponent.cpp
    (Modified for session state)

  ==============================================================================
*/

#include "MasterUtilityComponent.h"
#include "../../Components/Helpers.h"
#include "../Windows/QuickKeySettingsWindow.h" 
// <<< FIX: Added the full header include for MasterPluginsWindow >>>
#include "../Windows/MasterPluginsWindow.h" 
#include "../../Data/AppState.h"

MasterUtilityComponent::MasterUtilityComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    AppState::getInstance().addChangeListener(this);

    // <<< KHỞI TẠO COMPONENT MỚI >>>
    recorderComponent = std::make_unique<RecorderComponent>(audioEngine);
    addAndMakeVisible(*recorderComponent);

    soundboardComponent = std::make_unique<SoundboardComponent>(audioEngine);
    addAndMakeVisible(*soundboardComponent);

    addAndMakeVisible(masterPluginsButton);
    addAndMakeVisible(quickKeySettingsButton);

    masterPluginsButton.onClick = [this] {
        if (masterPluginsWindow == nullptr)
        {
            masterPluginsWindow = std::make_unique<MasterPluginsWindow>(
                audioEngine.getMasterProcessor(),
                [this] { masterPluginsWindow.reset(); }
            );
        }
        else
        {
            masterPluginsWindow->toFront(true);
        }
        };

    // <<< MODIFIED: This now safely manages the window's lifecycle >>>
    quickKeySettingsButton.onClick = [this]
        {
            if (quickKeySettingsWindow == nullptr)
            {
                // Create the window with a callback that resets this unique_ptr
                quickKeySettingsWindow = std::make_unique<QuickKeySettingsWindow>(
                    [this] { quickKeySettingsWindow.reset(); }
                );
            }
            else
            {
                quickKeySettingsWindow->toFront(true);
            }
        };

    addAndMakeVisible(masterVolumeLabel);
    addAndMakeVisible(masterMuteButton);
    addAndMakeVisible(masterVolumeSlider);
    addAndMakeVisible(masterLevelMeter);

    masterVolumeLabel.setJustificationType(juce::Justification::centredRight);

    masterMuteButton.setClickingTogglesState(true);
    masterMuteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.withAlpha(0.8f));
    masterMuteButton.onClick = [this]
        {
            if (masterProcessor != nullptr)
            {
                masterProcessor->setMuted(masterMuteButton.getToggleState());
                updateMuteButtonAppearance();
            }
        };

    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    masterVolumeSlider.setRange(-60.0, 6.0, 0.1);
    masterVolumeSlider.setValue(0.0);

    masterVolumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    masterVolumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.5f));
    masterVolumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2d2d2d));

    masterVolumeSlider.onValueChange = [this]
        {
            if (masterProcessor != nullptr)
                masterProcessor->setGain(static_cast<float>(masterVolumeSlider.getValue()));
        };

    updateTexts();
    updateLockState();
}

MasterUtilityComponent::~MasterUtilityComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    AppState::getInstance().removeChangeListener(this);
}

void MasterUtilityComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

void MasterUtilityComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    const int masterControlsHeight = 60;
    const int managementHeight = 40; // Giảm chiều cao vùng này một chút
    const int recorderHeight = 80;
    const int padding = 10;

    auto masterControlsArea = bounds.removeFromBottom(masterControlsHeight);
    bounds.removeFromBottom(padding);
    auto recorderArea = bounds.removeFromBottom(recorderHeight);
    bounds.removeFromBottom(padding);
    auto managementArea = bounds.removeFromBottom(managementHeight);
    bounds.removeFromBottom(padding);
    auto soundboardArea = bounds;

    soundboardComponent->setBounds(soundboardArea);
    recorderComponent->setBounds(recorderArea);

    // <<< SỬA LOGIC LAYOUT Ở ĐÂY >>>
    const int gap = 10;
    auto buttonBounds = managementArea;
    masterPluginsButton.setBounds(buttonBounds.removeFromLeft(buttonBounds.getWidth() / 2 - gap / 2));
    quickKeySettingsButton.setBounds(buttonBounds.withX(buttonBounds.getX() + gap));


    auto masterBounds = masterControlsArea;
    auto topRow = masterBounds.removeFromTop(masterBounds.getHeight() / 2);
    auto bottomRow = masterBounds;

    masterVolumeLabel.setBounds(topRow.removeFromLeft(100).reduced(5, 2));
    masterLevelMeter.setBounds(topRow.reduced(5, 2));

    masterMuteButton.setBounds(bottomRow.removeFromLeft(80).reduced(5, 2));
    bottomRow.removeFromLeft(5);
    masterVolumeSlider.setBounds(bottomRow.reduced(5, 2));
}

void MasterUtilityComponent::setMasterProcessor(MasterProcessor* proc)
{
    masterProcessor = proc;
    if (masterProcessor != nullptr)
    {
        masterVolumeSlider.setValue(masterProcessor->getGain(), juce::dontSendNotification);
        masterMuteButton.setToggleState(masterProcessor->isMuted(), juce::dontSendNotification);
        updateMuteButtonAppearance();
    }
}

LevelMeter& MasterUtilityComponent::getMasterLevelMeter()
{
    return masterLevelMeter;
}

juce::ValueTree MasterUtilityComponent::getOpenWindowsState() const
{
    juce::ValueTree state("OPEN_MASTER_WINDOWS");
    if (masterProcessor != nullptr)
        state.setProperty("processorId", masterProcessor->getState().getType().toString(), nullptr);
    return state;
}

void MasterUtilityComponent::restoreOpenWindows(const juce::ValueTree& state)
{
    juce::ignoreUnused(state);
}

void MasterUtilityComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    masterVolumeLabel.setText(lang.get("statusbar.masterVolumeLabel"), juce::dontSendNotification);
    masterPluginsButton.setButtonText(lang.get("masterUtility.masterPlugins"));
    quickKeySettingsButton.setButtonText(lang.get("masterUtility.quickKeySettings"));
    updateMuteButtonAppearance();
}

void MasterUtilityComponent::updateMuteButtonAppearance()
{
    auto& lang = LanguageManager::getInstance();
    if (masterProcessor != nullptr && masterMuteButton.getToggleState())
        masterMuteButton.setButtonText(lang.get("statusbar.mutedText"));
    else
        masterMuteButton.setButtonText(lang.get("statusbar.muteText"));
}

void MasterUtilityComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
        updateTexts();
    else if (source == &AppState::getInstance()) // <<< THÊM LOGIC
        updateLockState();
}

void MasterUtilityComponent::closeAllPluginWindows()
{
    // Nếu cửa sổ plugin kênh tổng đang mở, hãy đóng nó lại.
    // Thao tác này sẽ kích hoạt destructor và dọn dẹp các cửa sổ editor bên trong.
    if (masterPluginsWindow != nullptr)
    {
        masterPluginsWindow.reset();
    }
    // <<< ADDED: Also close the QuickKeySettingsWindow on shutdown >>>
    if (quickKeySettingsWindow != nullptr)
    {
        quickKeySettingsWindow.reset();
    }
}

void MasterUtilityComponent::updateLockState()
{
    const bool isLocked = AppState::getInstance().isSystemLocked();

    masterPluginsButton.setEnabled(!isLocked);
    quickKeySettingsButton.setEnabled(!isLocked);
    masterVolumeSlider.setEnabled(!isLocked);

    // Các thành phần biểu diễn vẫn hoạt động
    recorderComponent->setEnabled(true);
    soundboardComponent->setEnabled(true);
    masterMuteButton.setEnabled(true);
}