#include "FXChainWindow.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"
#include "../../Data/AppState.h"
#include "../../Data/PluginManager/PluginManager.h"

// ... (PluginWindow class is unchanged) ...
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


class FXChainWindow::Content;

// ... (PluginItemComponent class is unchanged) ...
class PluginItemComponent : public juce::Component, public juce::Button::Listener
{
public:
    PluginItemComponent(FXChainWindow::Content& owner, int row);
    ~PluginItemComponent() override = default;

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto reorderArea = bounds.removeFromRight(25);
        moveUpButton.setBounds(reorderArea.removeFromTop(reorderArea.getHeight() / 2));
        moveDownButton.setBounds(reorderArea);
        bounds.removeFromRight(5);
        auto controlsArea = bounds.removeFromRight(120);
        removeButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        controlsArea.removeFromRight(5);
        powerButton.setBounds(controlsArea.removeFromRight(50).reduced(2));
        controlsArea.removeFromRight(5);
        openButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        nameLabel.setBounds(bounds);
    }

    void buttonClicked(juce::Button* button) override;
    void update(int newRowNumber);

private:
    FXChainWindow::Content& ownerComponent;
    int rowNumber;
    juce::Label nameLabel;
    juce::TextButton openButton;
    juce::ToggleButton powerButton;
    juce::TextButton removeButton, moveUpButton, moveDownButton;
};


//==============================================================================
class FXChainWindow::Content : public juce::Component,
    public juce::Slider::Listener,
    public juce::ListBoxModel,
    public juce::ComboBox::Listener,
    public juce::ChangeListener,
    public juce::AudioProcessorListener
{
public:
    Content(ProcessorBase& processor) : processorToControl(processor)
    {
        processorToControl.addChangeListener(this);
        getSharedPluginManager().addChangeListener(this);
        LanguageManager::getInstance().addChangeListener(this); // <<< ADDED
        addListenerToAllPlugins();

        // <<< MODIFIED: Slider and Label setup >>>
        auto& lang = LanguageManager::getInstance();

        addAndMakeVisible(sendSlider);
        sendSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        sendSlider.setRange(0.0, 100.0, 0.1);
        sendSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        sendSlider.addListener(this);
        sendSlider.setValue(processorToControl.getSendLevel() * 100.0, juce::dontSendNotification);

        addAndMakeVisible(sendLabel);
        sendLabel.setText(lang.get("fxChain.send"), juce::dontSendNotification);
        sendLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(returnSlider);
        returnSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        returnSlider.setRange(0.0, 100.0, 0.1);
        returnSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        returnSlider.addListener(this);
        returnSlider.setValue(processorToControl.getReturnLevel() * 100.0, juce::dontSendNotification);

        addAndMakeVisible(returnLabel);
        returnLabel.setText(lang.get("fxChain.return"), juce::dontSendNotification);
        returnLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(pluginListLabel);
        pluginListLabel.setText(lang.get("tracks.activePlugins"), juce::dontSendNotification); // <<< MODIFIED
        pluginListLabel.setFont(IdolUIHelpers::createRegularFont(16.0f));

        addAndMakeVisible(pluginListBox);
        pluginListBox.setModel(this);
        pluginListBox.setRowHeight(35);
        pluginListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff212121));

        addAndMakeVisible(addPluginSelector);
        addPluginSelector.setEditableText(true);
        // <<< MODIFIED: Add placeholder text >>>
        addPluginSelector.setTextWhenNothingSelected(lang.get("tracks.addPluginPlaceholder"));
        addPluginSelector.addListener(this);
        updatePluginSelector();

        isUpdatingFromTextChange = false;

        addAndMakeVisible(addButton);
        addButton.setButtonText(lang.get("tracks.add")); // <<< MODIFIED
        addButton.onClick = [this] { addSelectedPlugin(); };

        setSize(500, 600);
    }

    ~Content() override
    {
        openPluginWindows.clear();
        processorToControl.removeChangeListener(this);
        getSharedPluginManager().removeChangeListener(this);
        LanguageManager::getInstance().removeChangeListener(this); // <<< ADDED
        removeListenerFromAllPlugins();
    }

    // <<< MODIFIED: paint() is simplified >>>
    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    // <<< MODIFIED: resized() is completely new >>>
    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);

        // Top area for Send/Return controls
        auto topControlsArea = bounds.removeFromTop(60);
        auto sendArea = topControlsArea.removeFromTop(topControlsArea.getHeight() / 2).reduced(0, 2);
        auto returnArea = topControlsArea.reduced(0, 2);

        sendLabel.setBounds(sendArea.removeFromLeft(60));
        sendSlider.setBounds(sendArea);

        returnLabel.setBounds(returnArea.removeFromLeft(60));
        returnSlider.setBounds(returnArea);

        bounds.removeFromTop(10);

        // Bottom area for plugin list
        auto pluginArea = bounds;
        pluginListLabel.setBounds(pluginArea.removeFromTop(30));

        auto addArea = pluginArea.removeFromBottom(30);
        pluginArea.removeFromBottom(10);

        pluginListBox.setBounds(pluginArea);

        addButton.setBounds(addArea.removeFromRight(80));
        addArea.removeFromRight(10);
        addPluginSelector.setBounds(addArea);
    }

    void sliderValueChanged(juce::Slider* slider) override
    {
        if (slider == &sendSlider)
            processorToControl.setSendLevel(static_cast<float>(slider->getValue()) / 100.0f);
        else if (slider == &returnSlider)
            processorToControl.setReturnLevel(static_cast<float>(slider->getValue()) / 100.0f);

        AppState::getInstance().setPresetDirty(true);
    }

    void showPluginEditor(int row)
    {
        if (auto* plugin = processorToControl.getPlugin(row))
        {
            if (!plugin->hasEditor()) return;

            for (auto* window : openPluginWindows)
                if (auto* pw = dynamic_cast<PluginWindow*>(window))
                    if (&pw->getOwnerPlugin() == plugin)
                    {
                        window->toFront(true);
                        return;
                    }

            if (auto* editor = plugin->createEditorIfNeeded())
                openPluginWindows.add(new PluginWindow(*plugin, editor, openPluginWindows));
        }
    }

    ProcessorBase& getProcessor() { return processorToControl; }

    void comboBoxChanged(juce::ComboBox* cb) override
    {
        if (isUpdatingFromTextChange) return; // <<< ADDED
        if (cb == &addPluginSelector && addPluginSelector.getSelectedId() == 0)
            updatePluginSelector(addPluginSelector.getText());
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &processorToControl)
        {
            pluginListBox.updateContent();
            removeListenerFromAllPlugins();
            addListenerToAllPlugins();
        }
        else if (source == &getSharedPluginManager())
        {
            updatePluginSelector();
        }
        // <<< ADDED: Handle language changes >>>
        else if (source == &LanguageManager::getInstance())
        {
            auto& lang = LanguageManager::getInstance();
            sendLabel.setText(lang.get("fxChain.send"), juce::dontSendNotification);
            returnLabel.setText(lang.get("fxChain.return"), juce::dontSendNotification);
            pluginListLabel.setText(lang.get("tracks.activePlugins"), juce::dontSendNotification);
            addButton.setButtonText(lang.get("tracks.add"));
            addPluginSelector.setTextWhenNothingSelected(lang.get("tracks.addPluginPlaceholder"));
        }
    }

    // ... (rest of the Content class is largely unchanged) ...
    int getNumRows() override { return processorToControl.getNumPlugins(); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow(int row, bool, juce::Component* existing) override
    {
        auto* item = static_cast<PluginItemComponent*>(existing);
        if (item == nullptr) item = new PluginItemComponent(*this, row);
        else item->update(row);
        return item;
    }

    void addSelectedPlugin()
    {
        const int selectedId = addPluginSelector.getSelectedId();
        if (selectedId == 0) return;

        for (const auto& desc : getSharedPluginManager().getKnownPlugins())
        {
            if (desc.uniqueId == selectedId)
            {
                auto spec = processorToControl.getProcessSpec();
                if (auto instance = getSharedPluginManager().createPluginInstance(desc, spec))
                {
                    processorToControl.addPlugin(std::move(instance));
                }
                addPluginSelector.setText({}, juce::dontSendNotification);
                updatePluginSelector();
                return;
            }
        }
    }

    void updatePluginSelector(const juce::String& searchText = {})
    {
        isUpdatingFromTextChange = true;
        const auto currentText = addPluginSelector.getText();
        const auto currentlySelectedId = addPluginSelector.getSelectedId();
        addPluginSelector.clear(juce::dontSendNotification);

        bool currentSelectionStillExists = false;
        for (const auto& desc : getSharedPluginManager().getFilteredPlugins(searchText))
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

    void addListenerToAllPlugins()
    {
        for (int i = 0; i < processorToControl.getNumPlugins(); ++i)
            if (auto* p = processorToControl.getPlugin(i)) p->addListener(this);
    }

    void removeListenerFromAllPlugins()
    {
        for (int i = 0; i < processorToControl.getNumPlugins(); ++i)
            if (auto* p = processorToControl.getPlugin(i)) p->removeListener(this);
    }

    void audioProcessorParameterChanged(juce::AudioProcessor*, int, float) override
    {
        AppState::getInstance().setPresetDirty(true);
        juce::MessageManager::callAsync([this] { pluginListBox.updateContent(); });
    }

    void audioProcessorChanged(juce::AudioProcessor*, const juce::AudioProcessor::ChangeDetails& d) override
    {
        if (d.programChanged)
        {
            AppState::getInstance().setPresetDirty(true);
            juce::MessageManager::callAsync([this] { pluginListBox.updateContent(); });
        }
    }

private:
    ProcessorBase& processorToControl;
    juce::Slider sendSlider, returnSlider;
    juce::Label sendLabel, returnLabel, pluginListLabel;
    juce::ListBox pluginListBox;
    juce::ComboBox addPluginSelector;
    juce::TextButton addButton;
    bool isUpdatingFromTextChange = false;
    juce::OwnedArray<juce::DocumentWindow> openPluginWindows;
};

// ... (PluginItemComponent implementation is unchanged) ...
PluginItemComponent::PluginItemComponent(FXChainWindow::Content& owner, int row) : ownerComponent(owner), rowNumber(row)
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

    update(row);
}

void PluginItemComponent::buttonClicked(juce::Button* button)
{
    if (button == &openButton) ownerComponent.showPluginEditor(rowNumber);
    else if (button == &powerButton) ownerComponent.getProcessor().setPluginBypassed(rowNumber, !ownerComponent.getProcessor().isPluginBypassed(rowNumber));
    else if (button == &removeButton) ownerComponent.getProcessor().removePlugin(rowNumber);
    else if (button == &moveUpButton) ownerComponent.getProcessor().movePlugin(rowNumber, rowNumber - 1);
    else if (button == &moveDownButton) ownerComponent.getProcessor().movePlugin(rowNumber, rowNumber + 1);
}

void PluginItemComponent::update(int newRowNumber)
{
    rowNumber = newRowNumber;
    if (auto* plugin = ownerComponent.getProcessor().getPlugin(rowNumber))
    {
        nameLabel.setText(juce::String(rowNumber + 1) + ". " + plugin->getName(), juce::dontSendNotification);
        const bool isBypassed = ownerComponent.getProcessor().isPluginBypassed(rowNumber);
        powerButton.setToggleState(!isBypassed, juce::dontSendNotification);
        powerButton.setButtonText(isBypassed ? "OFF" : "ON");
        openButton.setEnabled(plugin->hasEditor());
    }
}


// ... (FXChainWindow implementation is unchanged) ...
FXChainWindow::FXChainWindow(const juce::String& name, ProcessorBase& processorToControl)
    : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(450, 400, 1000, 1200);
    contentComponent = std::make_unique<Content>(processorToControl);
    setContentOwned(contentComponent.get(), true);
    centreWithSize(500, 450); // <<< MODIFIED: Reduced default height
    setVisible(true);
}

FXChainWindow::~FXChainWindow()
{
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer(this)]() mutable {
        if (safeThis)
            delete safeThis.getComponent();
        });
}

void FXChainWindow::closeButtonPressed()
{
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer(this)]() mutable {
        if (safeThis)
            delete safeThis.getComponent();
        });
}