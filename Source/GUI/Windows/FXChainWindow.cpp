#include "FXChainWindow.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"
#include "../../Data/AppState.h"
#include "../../Data/PluginManager/PluginManager.h"

// <<< ADDED: Helper class for floating plugin editor windows >>>
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

//==============================================================================
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
        // <<< MODIFIED: Adjusted width for new open button >>>
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
    // <<< ADDED: Open button >>>
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
        addListenerToAllPlugins();

        // ... (Slider and Label setup code is unchanged) ...
        addAndMakeVisible(sendSlider);
        sendSlider.setSliderStyle(juce::Slider::LinearBarVertical);
        sendSlider.setRange(0.0, 100.0, 0.1);
        sendSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        sendSlider.addListener(this);
        sendSlider.setValue(processorToControl.getSendLevel() * 100.0, juce::dontSendNotification);

        addAndMakeVisible(sendLabel);
        sendLabel.setText("Send", juce::dontSendNotification);
        sendLabel.setJustificationType(juce::Justification::centred);
        sendLabel.attachToComponent(&sendSlider, false);

        addAndMakeVisible(returnSlider);
        returnSlider.setSliderStyle(juce::Slider::LinearBarVertical);
        returnSlider.setRange(0.0, 100.0, 0.1);
        returnSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        returnSlider.addListener(this);
        returnSlider.setValue(processorToControl.getReturnLevel() * 100.0, juce::dontSendNotification);

        addAndMakeVisible(returnLabel);
        returnLabel.setText("Return", juce::dontSendNotification);
        returnLabel.setJustificationType(juce::Justification::centred);
        returnLabel.attachToComponent(&returnSlider, false);

        addAndMakeVisible(pluginListLabel);
        pluginListLabel.setText("Plugin Chain", juce::dontSendNotification);
        pluginListLabel.setFont(IdolUIHelpers::createRegularFont(16.0f));

        addAndMakeVisible(pluginListBox);
        pluginListBox.setModel(this);
        pluginListBox.setRowHeight(35);
        pluginListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff212121));

        addAndMakeVisible(addPluginSelector);
        addPluginSelector.setEditableText(true);
        addPluginSelector.addListener(this);
        updatePluginSelector();

        isUpdatingFromTextChange = false;

        addAndMakeVisible(addButton);
        addButton.setButtonText("Add");
        addButton.onClick = [this] { addSelectedPlugin(); };

        setSize(500, 600);
    }

    ~Content() override
    {
        // Xóa tất cả các cửa sổ editor plugin một cách đồng bộ.
        // Đây là cách làm đúng và an toàn.
        openPluginWindows.clear();

        // Hủy đăng ký các listener
        processorToControl.removeChangeListener(this);
        getSharedPluginManager().removeChangeListener(this);
        removeListenerFromAllPlugins();
    }

    // ... (paint, resized, sliderValueChanged, etc. are unchanged) ...
    void paint(juce::Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        auto bounds = getLocalBounds();
        auto pluginArea = bounds.removeFromRight(static_cast<int>(bounds.getWidth() * 0.66f));
        g.setColour(juce::Colours::grey);
        g.drawVerticalLine(pluginArea.getX(), bounds.getY() + 10.0f, bounds.getBottom() - 10.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        auto controlsArea = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.33f));
        auto pluginArea = bounds;
        pluginArea.removeFromLeft(10);

        auto sendArea = controlsArea.removeFromLeft(controlsArea.getWidth() / 2);
        auto returnArea = controlsArea;
        sendSlider.setBounds(sendArea.reduced(10, 30));
        returnSlider.setBounds(returnArea.reduced(10, 30));

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

    // ... (rest of the Content class methods) ...
    void comboBoxChanged(juce::ComboBox* cb) override
    {
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
    }

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
        // Thêm các biến isUpdatingFromTextChange để tránh vòng lặp vô hạn,
        // giống như cách TrackComponent đã làm.
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

        // <<< FIX IS HERE: Thêm logic để hiển thị popup >>>
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
    // <<< ADDED: To manage open plugin editor windows >>>
    juce::OwnedArray<juce::DocumentWindow> openPluginWindows;
};

// <<< MODIFIED: Implementation of PluginItemComponent with Open button >>>
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


//==============================================================================
FXChainWindow::FXChainWindow(const juce::String& name, ProcessorBase& processorToControl)
    : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(450, 400, 1000, 1200);
    contentComponent = std::make_unique<Content>(processorToControl);
    setContentOwned(contentComponent.get(), true);
    centreWithSize(500, 600);
    setVisible(true);
}

FXChainWindow::~FXChainWindow() { contentComponent.reset(); }

// <<< MODIFIED: Safe self-deletion >>>
void FXChainWindow::closeButtonPressed()
{
    // This safely schedules the deletion of this window on the message thread,
    // preventing crashes if the window is still being accessed.
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer(this)]() mutable {
        if (safeThis)
            delete safeThis.getComponent();
        });
}