#include "MasterPluginsWindow.h"
#include "../../Application/Application.h"
#include "../../Components/Helpers.h"


// A simple helper class to manage the lifecycle of the floating plugin window.
class PluginWindow final : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor& plugin, juce::AudioProcessorEditor* editor, juce::OwnedArray<juce::DocumentWindow>& windowList)
        : DocumentWindow(plugin.getName(),
            juce::Desktop::getInstance().getDefaultLookAndFeel()
            .findColour(juce::ResizableWindow::backgroundColourId),
            allButtons),
        ownerList(windowList)
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
            if (minW > 0 && minH > 0) setResizeLimits(minW, minH, maxW, maxH);
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

private:
    juce::OwnedArray<juce::DocumentWindow>& ownerList;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};


// ==============================================================================
// The component containing the actual UI for the master plugin editor
class MasterPluginsEditorComponent : public juce::Component,
    public juce::ChangeListener,
    public juce::ListBoxModel,
    public juce::AudioProcessorListener,
    public juce::ComboBox::Listener // <<< MODIFIED
{
public:
    MasterPluginsEditorComponent(MasterProcessor& proc);
    ~MasterPluginsEditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override; // <<< NEW

    class PluginItemComponent;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void audioProcessorParameterChanged(juce::AudioProcessor* p, int, float) override { juce::ignoreUnused(p); }
    void audioProcessorChanged(juce::AudioProcessor* p, const ChangeDetails& d) override;
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void addSelectedPlugin();
    void deletePlugin(int row);
    void showPluginEditor(int row);
    void togglePluginBypass(int row);
    void updateTexts();
    void updatePluginSelector(const juce::String& searchText = {}); // <<< MODIFIED
    void addListenerToAllPlugins();
    void removeListenerFromAllPlugins();

    MasterProcessor& masterProcessor;
    juce::OwnedArray<juce::DocumentWindow> openPluginWindows;
    juce::Label titleLabel, pluginListLabel;
    juce::ListBox pluginListBox;
    juce::ComboBox addPluginSelector;
    juce::TextButton addButton;

    bool isUpdatingFromTextChange = false; // <<< NEW
};

// ... (PluginItemComponent class remains unchanged) ...
class MasterPluginsEditorComponent::PluginItemComponent : public juce::Component, public juce::Button::Listener
{
public:
    PluginItemComponent(MasterPluginsEditorComponent& owner, int row) : ownerComponent(owner), rowNumber(row)
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

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto reorderArea = bounds.removeFromRight(25);
        moveUpButton.setBounds(reorderArea.removeFromTop(reorderArea.getHeight() / 2));
        moveDownButton.setBounds(reorderArea);
        bounds.removeFromRight(5);
        auto controlsArea = bounds.removeFromRight(115);
        removeButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        controlsArea.removeFromRight(5);
        powerButton.setBounds(controlsArea.removeFromRight(50).reduced(2));
        controlsArea.removeFromRight(5);
        openButton.setBounds(controlsArea.removeFromRight(30).reduced(2));
        nameLabel.setBounds(bounds);
    }

    void buttonClicked(juce::Button* button) override
    {
        if (button == &powerButton)      ownerComponent.togglePluginBypass(rowNumber);
        else if (button == &removeButton)    ownerComponent.deletePlugin(rowNumber);
        else if (button == &openButton)      ownerComponent.showPluginEditor(rowNumber);
        else if (button == &moveUpButton)
        {
            if (rowNumber > 0)
                ownerComponent.masterProcessor.movePlugin(rowNumber, rowNumber - 1);
        }
        else if (button == &moveDownButton)
        {
            if (rowNumber < ownerComponent.masterProcessor.getNumPlugins() - 1)
                ownerComponent.masterProcessor.movePlugin(rowNumber, rowNumber + 1);
        }
    }

    void update(int newRowNumber)
    {
        rowNumber = newRowNumber;
        if (auto* plugin = ownerComponent.masterProcessor.getPlugin(rowNumber))
        {
            nameLabel.setText(juce::String(rowNumber + 1) + ". " + plugin->getName(), juce::dontSendNotification);
            const bool isBypassed = ownerComponent.masterProcessor.isPluginBypassed(rowNumber);
            powerButton.setToggleState(!isBypassed, juce::dontSendNotification);
            powerButton.setButtonText(isBypassed ? "OFF" : "ON");
        }
    }

private:
    MasterPluginsEditorComponent& ownerComponent;
    int rowNumber;
    juce::Label nameLabel;
    juce::TextButton openButton, removeButton, moveUpButton, moveDownButton;
    juce::ToggleButton powerButton;
};


// ==============================================================================
// Implementation of MasterPluginsEditorComponent

MasterPluginsEditorComponent::MasterPluginsEditorComponent(MasterProcessor& proc) : masterProcessor(proc)
{
    getSharedPluginManager().addChangeListener(this);
    masterProcessor.addChangeListener(this);
    LanguageManager::getInstance().addChangeListener(this);
    addListenerToAllPlugins();

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(20.0f));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(pluginListLabel);
    pluginListLabel.setFont(IdolUIHelpers::createRegularFont(16.0f));

    addAndMakeVisible(pluginListBox);
    pluginListBox.setModel(this);
    pluginListBox.setRowHeight(35);
    pluginListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff212121));

    addAndMakeVisible(addPluginSelector);
    addPluginSelector.setEditableText(true);
    addPluginSelector.addListener(this);

    addAndMakeVisible(addButton);
    addButton.onClick = [this] { addSelectedPlugin(); };

    updateTexts();
    updatePluginSelector();
}

MasterPluginsEditorComponent::~MasterPluginsEditorComponent()
{
    openPluginWindows.clear();
    getSharedPluginManager().removeChangeListener(this);
    masterProcessor.removeChangeListener(this);
    LanguageManager::getInstance().removeChangeListener(this);
    removeListenerFromAllPlugins();
}

void MasterPluginsEditorComponent::paint(juce::Graphics& g) { g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId)); }

void MasterPluginsEditorComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    pluginListLabel.setBounds(bounds.removeFromTop(30));
    auto addArea = bounds.removeFromBottom(30);
    bounds.removeFromBottom(10);
    pluginListBox.setBounds(bounds);

    addButton.setBounds(addArea.removeFromRight(80));
    addArea.removeFromRight(10);
    addPluginSelector.setBounds(addArea);
}

// <<< NEW: Implement the listener callback >>>
void MasterPluginsEditorComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
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

void MasterPluginsEditorComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &getSharedPluginManager()) updatePluginSelector();
    else if (source == &masterProcessor) pluginListBox.updateContent();
    else if (source == &LanguageManager::getInstance()) updateTexts();
}

void MasterPluginsEditorComponent::audioProcessorChanged(juce::AudioProcessor* p, const ChangeDetails& details)
{
    juce::ignoreUnused(p);
    if (details.programChanged) juce::MessageManager::callAsync([this] { pluginListBox.updateContent(); });
}

int MasterPluginsEditorComponent::getNumRows() { return masterProcessor.getNumPlugins(); }

juce::Component* MasterPluginsEditorComponent::refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existing)
{
    juce::ignoreUnused(isRowSelected);
    auto* item = static_cast<PluginItemComponent*>(existing);
    if (item == nullptr)
        item = new PluginItemComponent(*this, rowNumber);
    else
        item->update(rowNumber);
    return item;
}

// <<< MODIFIED: Add plugin using stable uniqueId >>>
void MasterPluginsEditorComponent::addSelectedPlugin()
{
    const int selectedUniqueId = addPluginSelector.getSelectedId();
    if (selectedUniqueId == 0)
        return;

    const auto pluginList = getSharedPluginManager().getKnownPlugins();
    for (const auto& desc : pluginList)
    {
        if (desc.uniqueId == selectedUniqueId)
        {
            const auto spec = masterProcessor.getProcessSpec();
            if (auto instance = getSharedPluginManager().createPluginInstance(desc, spec))
                masterProcessor.addPlugin(std::move(instance));

            addPluginSelector.setText({}, juce::dontSendNotification);
            updatePluginSelector();
            return;
        }
    }
}

void MasterPluginsEditorComponent::deletePlugin(int row)
{
    if (auto* plugin = masterProcessor.getPlugin(row))
        for (int i = openPluginWindows.size(); --i >= 0;)
            if (openPluginWindows[i]->getName() == plugin->getName())
            {
                openPluginWindows.removeObject(openPluginWindows[i], true);
                break;
            }
    masterProcessor.removePlugin(row);
}

void MasterPluginsEditorComponent::showPluginEditor(int row)
{
    if (auto* plugin = masterProcessor.getPlugin(row))
    {
        if (!plugin->hasEditor()) return;
        for (auto* window : openPluginWindows)
            if (window->getName() == plugin->getName())
            {
                window->toFront(true);
                return;
            }
        if (auto* editor = plugin->createEditorIfNeeded())
            openPluginWindows.add(new PluginWindow(*plugin, editor, openPluginWindows));
    }
}

void MasterPluginsEditorComponent::togglePluginBypass(int row)
{
    const bool isBypassed = masterProcessor.isPluginBypassed(row);
    masterProcessor.setPluginBypassed(row, !isBypassed);
    pluginListBox.updateContent();
}

void MasterPluginsEditorComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    titleLabel.setText(lang.get("masterPluginsWindow.title"), juce::dontSendNotification);
    pluginListLabel.setText(lang.get("tracks.activePlugins"), juce::dontSendNotification);
    addButton.setButtonText(lang.get("tracks.add"));
    addPluginSelector.setTextWhenNothingSelected(lang.get("tracks.addPluginPlaceholder"));
}

// <<< MODIFIED: Handle filtering >>>
void MasterPluginsEditorComponent::updatePluginSelector(const juce::String& searchText)
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

void MasterPluginsEditorComponent::addListenerToAllPlugins()
{
    for (int i = 0; i < masterProcessor.getNumPlugins(); ++i)
        if (auto* plugin = masterProcessor.getPlugin(i))
            plugin->addListener(this);
}

void MasterPluginsEditorComponent::removeListenerFromAllPlugins()
{
    for (int i = 0; i < masterProcessor.getNumPlugins(); ++i)
        if (auto* plugin = masterProcessor.getPlugin(i))
            plugin->removeListener(this);
}

// ==============================================================================
// Implementation of MasterPluginsWindow

MasterPluginsWindow::MasterPluginsWindow(MasterProcessor& processor, std::function<void()> onWindowClosed)
    : DocumentWindow(LanguageManager::getInstance().get("masterPluginsWindow.title"),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        allButtons),
    onWindowClosedCallback(onWindowClosed)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(400, 500, 800, 1000);
    setContentOwned(new MasterPluginsEditorComponent(processor), true);
    centreWithSize(450, 600);
    setVisible(true);
}

MasterPluginsWindow::~MasterPluginsWindow() {}

void MasterPluginsWindow::closeButtonPressed()
{
    if (onWindowClosedCallback)
        onWindowClosedCallback();
}