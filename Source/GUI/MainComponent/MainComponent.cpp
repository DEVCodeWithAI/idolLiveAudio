/*
  ==============================================================================

    MainComponent.cpp
    (Final fix using std::function callback)

  ==============================================================================
*/
#include "MainComponent.h"
#include "../Layout/MenubarComponent.h"
#include "../Layout/PresetBarComponent.h"
#include "../Layout/TrackComponent.h"
#include "../Layout/MasterUtilityComponent.h"
#include "../Layout/PluginManagementComponent.h"
#include "../Layout/StatusBarComponent.h"
#include "../Layout/BeatManagerComponent.h"
#include "../../Data/SoundboardManager.h"
#include "../../AudioEngine/SoundPlayer.h"
#include "../../Application/Application.h"
#include "../../Data/AppState.h"
#include "../Components/ProjectManagerComponent.h"


#if JUCE_WINDOWS && JUCE_ASIO
#include <juce_audio_devices/juce_audio_devices.h>
#endif

class MainComponent::GlassPane : public juce::Component
{
public:
    GlassPane(std::function<void()> callbackOnClick)
        : onClick(std::move(callbackOnClick)) {
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::ignoreUnused(event);

        if (onClick)
            onClick();
    }

private:
    std::function<void()> onClick;
};

MainComponent::MainComponent()
    : deviceManager(std::make_unique<juce::AudioDeviceManager>()),
    audioEngine(*deviceManager)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    addKeyListener(commandManager.getKeyMappings());
    setWantsKeyboardFocus(true);
    commandManager.registerAllCommandsForTarget(this);

    getSharedSoundboardProfileManager().addChangeListener(this);


    deviceManager->addChangeListener(this);
    deviceManager->addAudioCallback(&audioEngine);

    // --- NEW LOGIC: Set the callback on the AudioEngine ---
    // This lambda will be executed on the message thread when the audio device
    // has successfully started.
    audioEngine.onDeviceStarted = [this]
        {
            DBG("Restore Preset Called! Callback received: Audio device has started. Loading post-device state.");
            // Now it's safe to load the rest of the session state.
            AppState::getInstance().loadPostDeviceState(*this);
        };

    projectManager = std::make_unique<ProjectManagerComponent>(audioEngine);
    menubar = std::make_unique<MenubarComponent>(*deviceManager, audioEngine);
    presetBar = std::make_unique<PresetBarComponent>(audioEngine);
    vocalTrack = std::make_unique<TrackComponent>("tracks.vocal", juce::Colour(0xff60a5fa), TrackComponent::ChannelType::Vocal);
    musicTrack = std::make_unique<TrackComponent>("tracks.music", juce::Colour(0xff4ade80), TrackComponent::ChannelType::Music);
    beatManager = std::make_unique<BeatManagerComponent>();
    masterUtilityColumn = std::make_unique<MasterUtilityComponent>(audioEngine);
    pluginManagement = std::make_unique<PluginManagementComponent>();
    statusBar = std::make_unique<StatusBarComponent>();

    addAndMakeVisible(*projectManager);
    addAndMakeVisible(*menubar);
    addAndMakeVisible(*presetBar);
    addAndMakeVisible(*vocalTrack);
    addAndMakeVisible(*musicTrack);
    addAndMakeVisible(*beatManager);
    addAndMakeVisible(*masterUtilityColumn);
    addAndMakeVisible(*pluginManagement);
    addAndMakeVisible(*statusBar);

    beatManager->onWantsToExpand = [this] { setBeatManagerExpanded(true); };
    beatManager->onBeatPlayed = [this] { setBeatManagerExpanded(false); };

    audioEngine.linkTrackComponents(*vocalTrack, *musicTrack);
    vocalTrack->setAudioEngine(&audioEngine, *deviceManager);
    musicTrack->setAudioEngine(&audioEngine, *deviceManager);
    audioEngine.setSelectedOutputChannels(0, 1);
    audioEngine.linkMasterComponents(*masterUtilityColumn);

    reloadHotkeysFromSlots();

    juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio, [this](bool granted)
        {
            if (granted)
                changeListenerCallback(deviceManager.get());
        });

    // <<< MODIFIED: Increased component size >>>
    setSize(1640, 1010);

    startTimerHz(2);
}

MainComponent::~MainComponent()
{
    getSharedSoundboardProfileManager().removeChangeListener(this);

    if (deviceManager)
    {
        deviceManager->removeAudioCallback(&audioEngine);
        deviceManager->removeChangeListener(this);
    }
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void MainComponent::attemptToClose(std::function<void(bool)> callback)
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

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, callback](int result)
            {
                if (result == 1) // Save As...
                {
                    if (presetBar)
                    {
                        presetBar->saveAsNewPreset([callback](bool /*wasSaved*/) {
                            if (callback) callback(true);
                            });
                    }
                    else
                    {
                        if (callback) callback(true);
                    }
                }
                else if (result == 2) // Don't Save
                {
                    if (callback) callback(true);
                }
                else
                {
                    if (callback) callback(false);
                }
            }), true);
    }
    else
    {
        if (callback) callback(true);
    }
}


void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == deviceManager.get())
    {
        // This callback is now only responsible for updating UI elements that
        // depend on the list of active channels, not for session restore logic.
        audioEngine.updateActiveInputChannels(*deviceManager);
    }
    else if (source == &getSharedSoundboardProfileManager())
    {
        commandManager.commandStatusChanged();
        reloadHotkeysFromSlots();
    }
}

void MainComponent::reloadHotkeysFromSlots()
{
    auto* keyMappings = commandManager.getKeyMappings();
    keyMappings->clearAllKeyPresses();
    const auto& slots = getSharedSoundboardProfileManager().getCurrentSlots();

    for (int i = 0; i < slots.size(); ++i)
    {
        const auto& slot = slots.getReference(i);
        if (!slot.isEmpty() && slot.hotkey.isValid())
        {
            keyMappings->addKeyPress(CommandIDs::playSound1 + i, slot.hotkey, 0);
        }
    }
    removeKeyListener(commandManager.getKeyMappings());
    addKeyListener(commandManager.getKeyMappings());
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return findFirstTargetParentComponent(); }

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    const juce::CommandID ids[] = {
        CommandIDs::playSound1, CommandIDs::playSound2, CommandIDs::playSound3,
        CommandIDs::playSound4, CommandIDs::playSound5, CommandIDs::playSound6,
        CommandIDs::playSound7, CommandIDs::playSound8, CommandIDs::playSound9
    };
    commands.addArray(ids, juce::numElementsInArray(ids));
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    int slotIndex = -1;
    switch (commandID)
    {
    case CommandIDs::playSound1: slotIndex = 0; break;
    case CommandIDs::playSound2: slotIndex = 1; break;
    case CommandIDs::playSound3: slotIndex = 2; break;
    case CommandIDs::playSound4: slotIndex = 3; break;
    case CommandIDs::playSound5: slotIndex = 4; break;
    case CommandIDs::playSound6: slotIndex = 5; break;
    case CommandIDs::playSound7: slotIndex = 6; break;
    case CommandIDs::playSound8: slotIndex = 7; break;
    case CommandIDs::playSound9: slotIndex = 8; break;
    default: return;
    }

    const auto& slots = getSharedSoundboardProfileManager().getCurrentSlots();
    if (!juce::isPositiveAndBelow(slotIndex, slots.size())) return;

    const auto& slot = slots.getReference(slotIndex);

    juce::String commandName = slot.displayName.isNotEmpty()
        ? slot.displayName
        : "Soundboard Slot " + juce::String(slotIndex + 1);

    result.setInfo(commandName, "Plays the sound assigned to slot " + juce::String(slotIndex + 1), "Soundboard", 0);
    result.setActive(!slot.isEmpty());
}

bool MainComponent::perform(const InvocationInfo& info)
{
    int slotIndex = -1;
    switch (info.commandID)
    {
    case CommandIDs::playSound1: slotIndex = 0; break;
    case CommandIDs::playSound2: slotIndex = 1; break;
    case CommandIDs::playSound3: slotIndex = 2; break;
    case CommandIDs::playSound4: slotIndex = 3; break;
    case CommandIDs::playSound5: slotIndex = 4; break;
    case CommandIDs::playSound6: slotIndex = 5; break;
    case CommandIDs::playSound7: slotIndex = 6; break;
    case CommandIDs::playSound8: slotIndex = 7; break;
    case CommandIDs::playSound9: slotIndex = 8; break;
    default: return false;
    }

    const auto& slots = getSharedSoundboardProfileManager().getCurrentSlots();
    if (!juce::isPositiveAndBelow(slotIndex, slots.size())) return false;

    const auto& slot = slots.getReference(slotIndex);
    if (!slot.isEmpty())
    {
        audioEngine.getSoundPlayer().play(slot.audioFile, slotIndex);
        return true;
    }

    return false;
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto totalBounds = getLocalBounds();
    const int padding = 5;

    // Các thanh trên cùng và dưới cùng không đổi
    auto menubarArea = totalBounds.removeFromTop(50);
    auto presetBarArea = totalBounds.removeFromTop(50);
    auto statusBarArea = totalBounds.removeFromBottom(40);

    if (menubar) menubar->setBounds(menubarArea.reduced(padding, 0));
    if (presetBar) presetBar->setBounds(presetBarArea.reduced(padding, 0));
    if (statusBar) statusBar->setBounds(statusBarArea.reduced(padding, 0));

    // Vùng làm việc chính
    auto mainArea = totalBounds.reduced(padding, 0);

    // Cột Trái không đổi
    auto rightColumnArea = mainArea.removeFromRight(mainArea.getWidth() / 3);
    mainArea.removeFromRight(padding);
    auto leftColumn = mainArea;

    // Layout Cột Trái (không thay đổi)
    const int projectManagerHeight = 60;
    const int pluginManagementHeight = 60;
    auto projectManagerArea = leftColumn.removeFromTop(projectManagerHeight);
    leftColumn.removeFromTop(padding);
    auto pluginManagementArea = leftColumn.removeFromBottom(pluginManagementHeight);
    leftColumn.removeFromBottom(padding);
    auto tracksArea = leftColumn;
    if (projectManager) projectManager->setBounds(projectManagerArea);
    if (pluginManagement) pluginManagement->setBounds(pluginManagementArea);
    if (vocalTrack) vocalTrack->setBounds(tracksArea.removeFromLeft(tracksArea.getWidth() / 2));
    tracksArea.removeFromLeft(padding);
    if (musicTrack) musicTrack->setBounds(tracksArea);

    // <<< SỬA: Layout ban đầu cho Cột Phải (trạng thái thu gọn) >>>
    const int beatManagerCompactHeight = 90; // Chiều cao khi thu gọn
    beatManager->setBounds(rightColumnArea.removeFromTop(beatManagerCompactHeight));
    rightColumnArea.removeFromTop(padding);
    masterUtilityColumn->setBounds(rightColumnArea);
}

void MainComponent::timerCallback()
{
    // Lấy thông tin từ deviceManager thay vì device
    if (auto* device = deviceManager->getCurrentAudioDevice())
    {
        // <<< FIX: Gọi getCpuUsage() từ deviceManager >>>
        auto cpuUsage = deviceManager->getCpuUsage() * 100.0;

        // Phần còn lại của hàm không thay đổi và đã đúng
        auto latencySamples = device->getInputLatencyInSamples() + device->getOutputLatencyInSamples();
        auto latencyMs = (latencySamples / device->getCurrentSampleRate()) * 1000.0;

        auto sampleRate = device->getCurrentSampleRate();

        if (statusBar != nullptr)
            statusBar->updateStatus(cpuUsage, latencyMs, sampleRate);
    }
    else
    {
        if (statusBar != nullptr)
            statusBar->updateStatus(0.0, 0.0, 0.0);
    }
}

// <<< THÊM HÀM MỚI NÀY VÀO FILE: Trái tim của cơ chế animation >>>
void MainComponent::setBeatManagerExpanded(bool shouldBeExpanded)
{
    if (shouldBeExpanded == isBeatManagerExpanded)
        return;

    isBeatManagerExpanded = shouldBeExpanded;

    if (shouldBeExpanded)
    {
        // Khi MỞ RỘNG: tạo và hiển thị "tấm kính"
        glassPane = std::make_unique<GlassPane>([this] { setBeatManagerExpanded(false); });
        addAndMakeVisible(*glassPane);
        glassPane->setBounds(getLocalBounds());

        // Đưa BeatManager lên phía trước để nó không bị "tấm kính" che mất
        beatManager->toFront(false);
    }
    else
    {
        // Khi THU GỌN: hủy bỏ "tấm kính"
        glassPane.reset();
    }

    // Phần animation còn lại giữ nguyên
    auto rightColumnArea = masterUtilityColumn->getBounds().getUnion(beatManager->getBounds());
    const int padding = 5;
    const int beatManagerCompactHeight = 90;
    const int beatManagerExpandedHeight = 640;

    juce::Rectangle<int> beatManagerTargetBounds;
    juce::Rectangle<int> masterUtilityTargetBounds;

    if (shouldBeExpanded)
    {
        beatManagerTargetBounds = rightColumnArea.removeFromTop(beatManagerExpandedHeight);
        rightColumnArea.removeFromTop(padding);
        masterUtilityTargetBounds = rightColumnArea;
    }
    else
    {
        this->grabKeyboardFocus();
        beatManagerTargetBounds = rightColumnArea.removeFromTop(beatManagerCompactHeight);
        rightColumnArea.removeFromTop(padding);
        masterUtilityTargetBounds = rightColumnArea;
    }

    const int animationTimeMs = 300;
    animator.animateComponent(beatManager.get(), beatManagerTargetBounds, 1.0f, animationTimeMs, false, 0.0, 0.0);
    animator.animateComponent(masterUtilityColumn.get(), masterUtilityTargetBounds, 1.0f, animationTimeMs, false, 0.0, 0.0);
}