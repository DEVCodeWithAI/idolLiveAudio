/*
  ==============================================================================

    TrackComponent.cpp
    (Layout Fix for FX Controls)

  ==============================================================================
*/
#include "GUI/Layout/TrackComponent.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Application/Application.h"
#include "../../Data/AppState.h"

namespace WindowStateIds
{
    const juce::Identifier OPEN_TRACK_WINDOWS("OPEN_TRACK_WINDOWS");
    const juce::Identifier WINDOW("WINDOW");
    const juce::Identifier processorId("processorId");
    const juce::Identifier uid("uid");
    const juce::Identifier x("x");
    const juce::Identifier y("y");
    const juce::Identifier width("w");
    const juce::Identifier height("h");
}

class PluginWindow final : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioPluginInstance& p, juce::AudioProcessorEditor* editor, juce::OwnedArray<juce::DocumentWindow>& windowList)
        : DocumentWindow(p.getName(),
            juce::Desktop::getInstance().getDefaultLookAndFeel()
            .findColour(juce::ResizableWindow::backgroundColourId),
            allButtons),
        ownerList(windowList),
        ownerPlugin(p)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setContentOwned(editor, true);

        if (auto* pluginConstrainer = editor->getConstrainer())
        {
            const int minW = pluginConstrainer->getMinimumWidth();
            const int minH = pluginConstrainer->getMinimumHeight();
            const int maxW = pluginConstrainer->getMaximumWidth();
            const int maxH = pluginConstrainer->getMaximumHeight();

            if (minW > 0 && minH > 0)
                setResizeLimits(minW, minH, maxW, maxH);
        }

        const int width = juce::jmax(300, editor->getWidth());
        const int height = juce::jmax(200, editor->getHeight());
        centreWithSize(width, height);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        ownerList.removeObject(this, true);
    }

    juce::AudioPluginInstance& getOwnerPlugin() const { return ownerPlugin; }

private:
    juce::OwnedArray<juce::DocumentWindow>& ownerList;
    juce::AudioPluginInstance& ownerPlugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};

class TrackComponent::PluginItemComponent : public juce::Component,
    public juce::Button::Listener
{
public:
    PluginItemComponent(TrackComponent& ownerComponent, int rowNumber)
        : owner(ownerComponent), row(rowNumber)
    {
        addAndMakeVisible(nameLabel);
        addAndMakeVisible(openButton);
        openButton.setButtonText("O");
        openButton.addListener(this);
        addAndMakeVisible(powerButton);
        powerButton.setClickingTogglesState(true);
        powerButton.addListener(this);
        powerButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        powerButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff4ade80));
        addAndMakeVisible(removeButton);
        removeButton.setButtonText("X");
        removeButton.addListener(this);
        removeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred.withAlpha(0.8f));
        addAndMakeVisible(moveUpButton);
        moveUpButton.setButtonText(juce::String::fromUTF8("↑"));
        moveUpButton.addListener(this);
        addAndMakeVisible(moveDownButton);
        moveDownButton.setButtonText(juce::String::fromUTF8("↓"));
        moveDownButton.addListener(this);
        setInterceptsMouseClicks(false, true);
        update(rowNumber);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto reorderArea = bounds.removeFromRight(25);
        moveUpButton.setBounds(reorderArea.removeFromTop(reorderArea.getHeight() / 2));
        moveDownButton.setBounds(reorderArea);
        bounds.removeFromRight(5);

        // Tăng chiều ngang của vùng chứa các nút
        auto controlsArea = bounds.removeFromRight(150);

        removeButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        controlsArea.removeFromRight(5);
        powerButton.setBounds(controlsArea.removeFromRight(50).reduced(2));
        controlsArea.removeFromRight(5);
        openButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        nameLabel.setBounds(bounds);
    }

    void buttonClicked(juce::Button* button) override
    {
        if (button == &powerButton)
            owner.togglePluginBypass(row);
        else if (button == &removeButton)
            owner.deletePlugin(row);
        else if (button == &openButton)
            owner.showPluginEditor(row);
        else if (button == &moveUpButton)
        {
            if (row > 0 && owner.processor != nullptr)
                owner.processor->movePlugin(row, row - 1);
        }
        else if (button == &moveDownButton)
        {
            if (owner.processor != nullptr && row < owner.processor->getNumPlugins() - 1)
                owner.processor->movePlugin(row, row + 1);
        }
    }

    void update(int newRowNumber)
    {
        row = newRowNumber;
        if (owner.processor == nullptr) return;

        if (auto* plugin = owner.processor->getPlugin(row))
        {
            nameLabel.setText(juce::String(row + 1) + ". " + plugin->getName(), juce::dontSendNotification);
            const bool isBypassed = owner.processor->isPluginBypassed(row);
            powerButton.setToggleState(!isBypassed, juce::dontSendNotification);
            powerButton.setButtonText(isBypassed ? "OFF" : "ON");
        }
    }

    void setLocked(bool shouldBeLocked, bool isSpecialSlot)
    {
        if (!isSpecialSlot)
        {
            setEnabled(!shouldBeLocked);
            return;
        }

        if (shouldBeLocked)
        {
            setEnabled(true);
            openButton.setEnabled(true);
            powerButton.setEnabled(false);
            removeButton.setEnabled(false);
            moveUpButton.setEnabled(false);
            moveDownButton.setEnabled(false);
        }
        else
        {
            setEnabled(true);
            openButton.setEnabled(true);
            powerButton.setEnabled(true);
            removeButton.setEnabled(true);
            moveUpButton.setEnabled(true);
            moveDownButton.setEnabled(true);
        }
    }
private:
    TrackComponent& owner;
    int row;
    juce::Label nameLabel;
    juce::TextButton openButton;
    juce::ToggleButton powerButton;
    juce::TextButton removeButton;
    juce::TextButton moveUpButton;
    juce::TextButton moveDownButton;
};

TrackComponent::TrackComponent(const juce::String& trackNameKey, const juce::Colour& headerColour, ChannelType type)
    : nameKey(trackNameKey), colour(headerColour), channelType(type)
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedPluginManager().addChangeListener(this);
    AppState::getInstance().addChangeListener(this);

    // Logic cho ListBox plugin không đổi
    pluginListBox.setModel(this);
    pluginListBox.setRowHeight(35);

    addAndMakeVisible(trackLabel);

    // <<< XÓA: Khởi tạo các control input cũ >>>

    addAndMakeVisible(lockButton);
    lockButton.setClickingTogglesState(true);
    lockButton.onClick = [this] { handleLockButtonClicked(); };
    addAndMakeVisible(muteButton);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(levelMeter);

    fxSends = std::make_unique<FxSendsComponent>(*this);
    addAndMakeVisible(*fxSends);

    addAndMakeVisible(pluginListLabel);
    addAndMakeVisible(pluginListBox);

    // Logic cho ComboBox add plugin không đổi
    addAndMakeVisible(addPluginSelector);
    addPluginSelector.setEditableText(true);
    addPluginSelector.addListener(this);

    addAndMakeVisible(addButton);
    addButton.onClick = [this] { addSelectedPlugin(); };

    trackLabel.setFont(IdolUIHelpers::createBoldFont(18.0f));
    trackLabel.setJustificationType(juce::Justification::centred);
    trackLabel.setColour(juce::Label::backgroundColourId, colour);
    trackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    pluginListLabel.setFont(IdolUIHelpers::createRegularFont(16.0f));
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.setRange(-60.0, 6.0, 0.1);
    volumeSlider.setValue(0.0);

    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2d2d2d));

    volumeSlider.onValueChange = [this]
        {
            if (processor != nullptr)
                processor->setGain(static_cast<float>(volumeSlider.getValue()));
        };
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.withAlpha(0.8f));
    muteButton.onClick = [this]
        {
            if (processor != nullptr)
            {
                processor->setMuted(muteButton.getToggleState());
                updateMuteButtonState();
            }
        };
    updateTexts();
    updatePluginSelector();
    updateLockState();
}

TrackComponent::~TrackComponent()
{
    closeAllPluginWindows();

    LanguageManager::getInstance().removeChangeListener(this);
    getSharedPluginManager().removeChangeListener(this);
    AppState::getInstance().removeChangeListener(this);

    if (processor)
    {
        removeListenerFromAllPlugins();
        processor->removeChangeListener(this);
    }
}


void TrackComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);

    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(40 + 10);

    // <<< SỬA: Chỉ cần vẽ 2 vùng nền chính >>>
    const int topSectionHeight = 170; // Chiều cao mới cho vùng trên cùng
    auto topSectionBounds = bounds.removeFromTop(topSectionHeight);

    g.setColour(juce::Colour(0xff212121));
    g.fillRoundedRectangle(topSectionBounds.toFloat(), 8.0f);
    bounds.removeFromTop(10);
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(topSectionBounds.toFloat(), 8.0f, 1.0f);
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);
}

// <<< SỬA: Viết lại hoàn toàn hàm resized() để layout chính xác >>>
void TrackComponent::resized()
{
    auto bounds = getLocalBounds();
    const int padding = 10;
    bounds.reduce(padding, padding);

    const int headerHeight = 40;
    const int topControlsHeight = 170;
    const int playerHeight = 90;

    auto headerArea = bounds.removeFromTop(headerHeight);
    bounds.removeFromTop(padding);
    auto topControlsArea = bounds.removeFromTop(topControlsHeight);
    bounds.removeFromTop(padding);
    auto bottomArea = bounds;

    trackLabel.setBounds(headerArea);

    // 1. Layout cho vùng điều khiển trên cùng
    {
        auto area = topControlsArea.reduced(padding, 0);

        const int fxHeight = 65;
        auto fxArea = area.removeFromBottom(fxHeight);
        fxSends->setBounds(fxArea);

        area.removeFromBottom(10);

        auto inputArea = area.reduced(0, 5);

        const int rowHeight = 30;

        // <<< THAY THẾ CONTROL CŨ BẰNG COMPONENT MỚI >>>
        auto inputLine1 = inputArea.removeFromTop(rowHeight);
        if (channelSelector)
            channelSelector->setBounds(inputLine1);

        inputArea.removeFromTop(5);

        auto inputLine2 = inputArea.removeFromTop(rowHeight);
        lockButton.setBounds(inputLine2.removeFromLeft(30));
        inputLine2.removeFromLeft(padding);
        muteButton.setBounds(inputLine2.removeFromLeft(80));
        inputLine2.removeFromLeft(padding);
        volumeSlider.setBounds(inputLine2.reduced(0, 5));

        inputArea.removeFromTop(8);
        levelMeter.setBounds(inputArea);
    }

    // 2. Layout cho vùng dưới cùng (không thay đổi nhiều)
    {
        auto playerArea = bottomArea.removeFromBottom(playerHeight);
        bottomArea.removeFromBottom(padding);
        auto pluginArea = bottomArea;

        if (trackPlayer)
            trackPlayer->setBounds(playerArea);

        auto pluginBounds = pluginArea;
        pluginListLabel.setBounds(pluginBounds.removeFromTop(30));

        auto addPluginArea = pluginBounds.removeFromBottom(30);
        pluginBounds.removeFromBottom(padding);

        auto indentedBounds = pluginBounds.reduced(5, 0);
        pluginListBox.setBounds(indentedBounds);

        auto indentedAddArea = addPluginArea.reduced(5, 0);
        addButton.setBounds(indentedAddArea.removeFromRight(80));
        indentedAddArea.removeFromRight(padding);
        addPluginSelector.setBounds(indentedAddArea);
    }
}

void TrackComponent::openFxWindow(int index)
{
    if (!isPositiveAndBelow(index, 4) || audioEngine == nullptr)
        return;

    // Đóng cửa sổ cũ nếu đang mở để tránh trùng lặp
    if (fxWindows[index] != nullptr)
    {
        fxWindows[index]->toFront(true);
        return;
    }

    ProcessorBase* targetFxProcessor = (channelType == ChannelType::Vocal)
                                     ? audioEngine->getFxProcessorForVocal(index)
                                     : audioEngine->getFxProcessorForMusic(index);

    if (targetFxProcessor == nullptr) return;

    auto windowName = trackLabel.getText() + " - FX " + juce::String(index + 1);
    fxWindows[index] = new FXChainWindow(windowName, *targetFxProcessor);
}

void TrackComponent::toggleFxMute(int index, bool shouldBeMuted)
{
    if (!isPositiveAndBelow(index, 4) || audioEngine == nullptr)
        return;
    
    ProcessorBase* fxProcessor = (channelType == ChannelType::Vocal)
                               ? audioEngine->getFxProcessorForVocal(index)
                               : audioEngine->getFxProcessorForMusic(index);

    if (fxProcessor != nullptr)
    {
        fxProcessor->setMuted(shouldBeMuted);
        // AppState::getInstance().setPresetDirty(true);
    }
}

void TrackComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // <<< LOGIC CŨ ĐƯỢC DỌN DẸP >>>
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
    else if (source == &getSharedPluginManager())
    {
        updatePluginSelector();
    }
    else if (source == processor)
    {
        pluginListBox.updateContent();
        removeListenerFromAllPlugins();
        addListenerToAllPlugins();
    }
    else if (source == &AppState::getInstance())
    {
        updateLockState();
    }
}

void TrackComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (isUpdatingFromTextChange)
        return;

    if (comboBoxThatHasChanged == &addPluginSelector)
    {
        if (addPluginSelector.getSelectedId() == 0)
        {
            updatePluginSelector(addPluginSelector.getText());
        }
    }
}

void TrackComponent::setProcessor(ProcessorBase* proc)
{
    if (processor)
    {
        removeListenerFromAllPlugins();
        processor->removeChangeListener(this);
    }
    processor = proc;
    if (processor)
    {
        processor->addChangeListener(this);
        addListenerToAllPlugins();
    }
    pluginListBox.updateContent();
    updateMuteButtonState();
}

void TrackComponent::addSelectedPlugin()
{
    if (processor == nullptr) return;

    const int selectedUniqueId = addPluginSelector.getSelectedId();
    if (selectedUniqueId == 0)
        return;

    const auto pluginList = getSharedPluginManager().getKnownPlugins();

    for (const auto& desc : pluginList)
    {
        if (desc.uniqueId == selectedUniqueId)
        {
            const auto spec = processor->getProcessSpec();
            try
            {
                if (auto instance = getSharedPluginManager().createPluginInstance(desc, spec))
                {
                    instance->addListener(this);
                    processor->addPlugin(std::move(instance));
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                        "Plugin Error", "Failed to create plugin instance.");
                }
            }
            catch (const std::exception& e)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Plugin Exception", "Error: " + juce::String(e.what()));
            }
            catch (...)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Plugin Exception", "An unknown error occurred while adding plugin.");
            }

            addPluginSelector.setText({}, juce::dontSendNotification);
            updatePluginSelector();
            return;
        }
    }
}

void TrackComponent::updatePluginSelector(const juce::String& searchText)
{
    isUpdatingFromTextChange = true;

    const auto currentText = addPluginSelector.getText();
    const auto currentlySelectedId = addPluginSelector.getSelectedId();

    addPluginSelector.clear(juce::dontSendNotification);

    const auto pluginList = getSharedPluginManager().getFilteredPlugins(searchText);

    bool currentSelectionStillExists = false;
    for (const auto& desc : pluginList)
    {
        addPluginSelector.addItem(desc.name, desc.uniqueId);
        if (desc.uniqueId == currentlySelectedId)
            currentSelectionStillExists = true;
    }

    if (currentSelectionStillExists)
        addPluginSelector.setSelectedId(currentlySelectedId, juce::dontSendNotification);

    addPluginSelector.setText(currentText, juce::dontSendNotification);

    if (searchText.isNotEmpty())
        addPluginSelector.showPopup();
    else
        addPluginSelector.hidePopup();

    isUpdatingFromTextChange = false;
}

void TrackComponent::deletePlugin(int row)
{
    if (processor == nullptr) return;

    if (auto* plugin = processor->getPlugin(row))
    {
        plugin->removeListener(this);
        for (int i = openPluginWindows.size(); --i >= 0;)
        {
            if (auto* pw = dynamic_cast<PluginWindow*>(openPluginWindows.getUnchecked(i)))
                if (&pw->getOwnerPlugin() == plugin)
                {
                    openPluginWindows.removeObject(pw, true);
                    break;
                }
        }
    }
    processor->removePlugin(row);
}

void TrackComponent::showPluginEditor(int row)
{
    if (processor == nullptr) return;
    if (auto* plugin = processor->getPlugin(row))
    {
        if (!plugin->hasEditor())
            return;
        for (auto* window : openPluginWindows)
        {
            if (auto* pw = dynamic_cast<PluginWindow*>(window))
                if (&pw->getOwnerPlugin() == plugin)
                {
                    window->toFront(true);
                    return;
                }
        }
        try
        {
            if (auto* editor = plugin->createEditorIfNeeded())
            {
                auto* newWindow = new PluginWindow(*plugin, editor, openPluginWindows);
                openPluginWindows.add(newWindow);
            }
        }
        catch (...)
        {
            auto& lang = LanguageManager::getInstance();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                lang.get("alerts.pluginErrorTitle"),
                lang.get("alerts.pluginCrashUnknown"));
        }
    }
}

LevelMeter& TrackComponent::getLevelMeter() { return levelMeter; }

void TrackComponent::setAudioEngine(AudioEngine* engine, juce::AudioDeviceManager& manager)
{
    audioEngine = engine;
    if (audioEngine == nullptr) return;

    if (trackPlayer == nullptr)
    {
        TrackPlayerComponent::PlayerType playerType = (channelType == ChannelType::Vocal) ?
            TrackPlayerComponent::PlayerType::Vocal :
            TrackPlayerComponent::PlayerType::Music;
        trackPlayer = std::make_unique<TrackPlayerComponent>(playerType, *audioEngine);
        addAndMakeVisible(*trackPlayer);
    }

    if (channelSelector == nullptr)
    {
        auto type = (channelType == ChannelType::Vocal)
            ? ChannelSelectorComponent::ChannelType::AudioInputMono
            : ChannelSelectorComponent::ChannelType::AudioInputStereo;

        // Truyền vào 'manager' thay vì 'audioEngine->getAudioDeviceManager()'
        channelSelector = std::make_unique<ChannelSelectorComponent>(manager, *audioEngine, type, "tracks.input");
        addAndMakeVisible(*channelSelector);

        channelSelector->onSelectionChange = [this](int newChannelIndex)
            {
                if (audioEngine)
                {
                    if (channelType == ChannelType::Vocal)
                        audioEngine->setVocalInputChannel(newChannelIndex);
                    else
                        audioEngine->setMusicInputChannel(newChannelIndex);
                }
            };
    }

    resized();
}

void TrackComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    trackLabel.setText(lang.get(nameKey), juce::dontSendNotification);
    pluginListLabel.setText(lang.get("tracks.activePlugins"), juce::dontSendNotification);
    addButton.setButtonText(lang.get("tracks.add"));
    addPluginSelector.setTextWhenNothingSelected(lang.get("tracks.addPluginPlaceholder"));
    updateMuteButtonState();
    if (audioEngine)
    {
        for (int i = 0; i < 4; ++i)
        {
            ProcessorBase* fxProcessor = (channelType == ChannelType::Vocal)
                ? audioEngine->getFxProcessorForVocal(i)
                : audioEngine->getFxProcessorForMusic(i);
            if (fxProcessor)
            {
                fxSends->updateMuteButtonState(i, fxProcessor->isMuted());
            }
        }
    }
    repaint();
}

void TrackComponent::togglePluginBypass(int row)
{
    if (processor == nullptr) return;
    const bool isCurrentlyBypassed = processor->isPluginBypassed(row);
    processor->setPluginBypassed(row, !isCurrentlyBypassed);
    if (auto* itemComponent = pluginListBox.getComponentForRowNumber(row))
    {
        if (auto* pluginItem = dynamic_cast<PluginItemComponent*>(itemComponent))
        {
            pluginItem->update(row);
        }
    }
}

void TrackComponent::audioProcessorParameterChanged(juce::AudioProcessor* changedProcessor, int, float)
{
    juce::ignoreUnused(changedProcessor);
    AppState::getInstance().setPresetDirty(true);
    juce::MessageManager::callAsync([this] { pluginListBox.updateContent(); });
}

void TrackComponent::audioProcessorChanged(juce::AudioProcessor* changedProcessor, const juce::AudioProcessor::ChangeDetails& details)
{
    juce::ignoreUnused(changedProcessor);
    if (details.programChanged)
    {
        AppState::getInstance().setPresetDirty(true);
        juce::MessageManager::callAsync([this] { pluginListBox.updateContent(); });
    }
}

void TrackComponent::addListenerToAllPlugins()
{
    if (processor == nullptr) return;
    for (int i = 0; i < processor->getNumPlugins(); ++i)
        if (auto* plugin = processor->getPlugin(i))
            plugin->addListener(this);
}

void TrackComponent::removeListenerFromAllPlugins()
{
    if (processor == nullptr) return;
    for (int i = 0; i < processor->getNumPlugins(); ++i)
        if (auto* plugin = processor->getPlugin(i))
            plugin->removeListener(this);
}

int TrackComponent::getNumRows()
{
    if (processor != nullptr) return processor->getNumPlugins();
    return 0;
}

void TrackComponent::paintListBoxItem(int, juce::Graphics&, int, int, bool) {}

juce::Component* TrackComponent::refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    juce::ignoreUnused(isRowSelected);
    auto* item = static_cast<PluginItemComponent*>(existingComponentToUpdate);
    if (item == nullptr)
        item = new PluginItemComponent(*this, rowNumber);
    else
        item->update(rowNumber);
    return item;
}

void TrackComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    pluginListBox.selectRow(row);
}

void TrackComponent::updateMuteButtonState()
{
    if (processor == nullptr)
        return;
    auto& lang = LanguageManager::getInstance();
    const bool isMuted = processor->isMuted();
    muteButton.setToggleState(isMuted, juce::dontSendNotification);
    muteButton.setButtonText(isMuted ? lang.get("tracks.muted") : lang.get("tracks.mute"));
}

juce::ValueTree TrackComponent::getOpenWindowsState() const
{
    juce::ValueTree state(WindowStateIds::OPEN_TRACK_WINDOWS);
    if (processor != nullptr)
        state.setProperty(WindowStateIds::processorId, processor->getState().getType().toString(), nullptr);

    for (auto* window : openPluginWindows)
    {
        if (auto* pluginWindow = dynamic_cast<PluginWindow*>(window))
        {
            juce::ValueTree windowState(WindowStateIds::WINDOW);
            auto& plugin = pluginWindow->getOwnerPlugin();
            windowState.setProperty(WindowStateIds::uid, (int)plugin.getPluginDescription().uniqueId, nullptr);
            auto bounds = pluginWindow->getBounds();
            windowState.setProperty(WindowStateIds::x, bounds.getX(), nullptr);
            windowState.setProperty(WindowStateIds::y, bounds.getY(), nullptr);
            windowState.setProperty(WindowStateIds::width, bounds.getWidth(), nullptr);
            windowState.setProperty(WindowStateIds::height, bounds.getHeight(), nullptr);
            state.addChild(windowState, -1, nullptr);
        }
    }
    return state;
}

void TrackComponent::restoreOpenWindows(const juce::ValueTree& state)
{
    if (processor == nullptr || !state.isValid())
        return;

    if (processor->getState().getType().toString() != state.getProperty(WindowStateIds::processorId, "").toString())
        return;

    for (const auto& windowState : state)
    {
        if (windowState.hasType(WindowStateIds::WINDOW))
        {
            const int uid = windowState.getProperty(WindowStateIds::uid, 0);
            if (uid == 0) continue;

            for (int i = 0; i < processor->getNumPlugins(); ++i)
            {
                if (auto* plugin = processor->getPlugin(i))
                {
                    if ((int)plugin->getPluginDescription().uniqueId == uid)
                    {
                        showPluginEditor(i);
                        juce::MessageManager::callAsync([this, windowState]
                            {
                                if (auto* window = openPluginWindows.getLast())
                                {
                                    const int x = windowState.getProperty(WindowStateIds::x, 100);
                                    const int y = windowState.getProperty(WindowStateIds::y, 100);
                                    const int w = windowState.getProperty(WindowStateIds::width, 500);
                                    const int h = windowState.getProperty(WindowStateIds::height, 400);
                                    window->setBounds(x, y, w, h);
                                }
                            });
                            break;
                    }
                }
            }
        }
    }
}

void TrackComponent::closeAllPluginWindows()
{
    openPluginWindows.clear();

    for (auto& fxWindowPtr : fxWindows)
    {
        if (auto* window = fxWindowPtr.getComponent())
        {
            delete window;
        }
    }
}

void TrackComponent::handleLockButtonClicked()
{
    auto& appState = AppState::getInstance();
    auto& lang = LanguageManager::getInstance();

    if (appState.isSystemLocked())
    {
        // Mở khóa
        auto* alert = new juce::AlertWindow(lang.get("lock.unlockTitle"),
            lang.get("lock.unlockMessage"),
            juce::AlertWindow::QuestionIcon);

        // <<< SỬA LỖI Ở ĐÂY: Thêm tham số 'true' để ẩn mật khẩu >>>
        alert->addTextEditor("password", "", lang.get("lock.passwordLabel"), true);

        alert->addButton(lang.get("lock.unlockButton"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton(lang.get("lock.cancelButton"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alert->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, &appState, alert, &lang](int result)
            {
                if (result == 1)
                {
                    if (!appState.unlockSystem(alert->getTextEditorContents("password")))
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                            lang.get("lock.errorTitle"),
                            lang.get("lock.incorrectPassword"));
                        lockButton.setToggleState(true, juce::dontSendNotification);
                    }
                }
                else
                {
                    lockButton.setToggleState(true, juce::dontSendNotification);
                }
            }), true);
    }
    else
    {
        // Khóa
        auto* alert = new juce::AlertWindow(lang.get("lock.lockTitle"),
            lang.get("lock.lockMessage"),
            juce::AlertWindow::QuestionIcon);
        alert->addTextEditor("password", "", lang.get("lock.passwordLabel4Digits"), true);
        alert->addButton(lang.get("lock.lockButton"), 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton(lang.get("lock.cancelButton"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alert->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, &appState, alert, &lang](int result)
            {
                if (result == 1)
                {
                    auto password = alert->getTextEditorContents("password");
                    if (password.isNotEmpty() && password.length() == 4 && password.containsOnly("0123456789"))
                    {
                        appState.setSystemLocked(true, password);
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                            lang.get("lock.errorTitle"),
                            lang.get("lock.invalidPassword"));
                        lockButton.setToggleState(false, juce::dontSendNotification);
                    }
                }
                else
                {
                    lockButton.setToggleState(false, juce::dontSendNotification);
                }
            }), true);
    }
}

void TrackComponent::updateLockState()
{
    const bool isLocked = AppState::getInstance().isSystemLocked();

    lockButton.setToggleState(isLocked, juce::dontSendNotification);

    if (isLocked)
    {
        lockButton.setButtonText("L");
        lockButton.setTooltip("System is Locked. Click to unlock.");
    }
    else
    {
        lockButton.setButtonText("U");
        lockButton.setTooltip("System is Unlocked. Click to lock.");
    }

    if (channelSelector)
        channelSelector->setEnabled(!isLocked);

    volumeSlider.setEnabled(!isLocked);
    addPluginSelector.setEnabled(!isLocked);
    addButton.setEnabled(!isLocked);

    fxSends->setLocked(isLocked);

    muteButton.setEnabled(true); // Mute button is always enabled

    pluginListBox.setEnabled(true);

    for (int i = 0; i < getNumRows(); ++i)
    {
        if (auto* item = dynamic_cast<PluginItemComponent*>(pluginListBox.getComponentForRowNumber(i)))
        {
            const bool isSpecialSlot = (channelType == ChannelType::Music && i == 0);

            item->setLocked(isLocked, isSpecialSlot);
        }
    }
}