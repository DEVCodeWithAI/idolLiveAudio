/*
  ==============================================================================

    PluginManagerWindow.cpp
    (FIXED)

  ==============================================================================
*/

#include "PluginManagerWindow.h"
#include "../../Components/Helpers.h"
#include "../../Application/Application.h"

// --- PluginManagerContentComponent Implementation ---

PluginManagerContentComponent::PluginManagerContentComponent()
{
    getSharedPluginManager().addChangeListener(this);
    LanguageManager::getInstance().addChangeListener(this);

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(18.0f));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(pluginListBox);
    pluginListBox.setModel(this);
    pluginListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff212121));

    addAndMakeVisible(scanButton);
    scanButton.onClick = [] {
        getSharedPluginManager().scanAndMergeAll();
        };

    addAndMakeVisible(addButton);
    addButton.onClick = [] { getSharedPluginManager().addPluginFromUserChoice(); };

    addAndMakeVisible(removeButton);
    removeButton.onClick = [this] { removeSelectedPlugin(); };

    updateTexts();
}

PluginManagerContentComponent::~PluginManagerContentComponent()
{
    getSharedPluginManager().removeChangeListener(this);
    LanguageManager::getInstance().removeChangeListener(this);
}

void PluginManagerContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PluginManagerContentComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    auto buttonArea = bounds.removeFromBottom(40);
    bounds.removeFromBottom(10);
    pluginListBox.setBounds(bounds);
    juce::FlexBox fb;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.items.add(juce::FlexItem(scanButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10));
    fb.items.add(juce::FlexItem(addButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10));
    fb.items.add(juce::FlexItem(removeButton).withFlex(1.0f));
    fb.performLayout(buttonArea);
}

int PluginManagerContentComponent::getNumRows()
{
    return getSharedPluginManager().getKnownPlugins().size();
}

void PluginManagerContentComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::blue.withAlpha(0.3f));
    g.setColour(juce::Colours::white);

    auto plugins = getSharedPluginManager().getKnownPlugins();

    if (isPositiveAndBelow(rowNumber, plugins.size()))
    {
        const auto& desc = plugins.getReference(rowNumber);
        g.drawText(desc.name, 5, 0, width - 10, height, juce::Justification::centredLeft, true);
    }
}

void PluginManagerContentComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &getSharedPluginManager() || source == &LanguageManager::getInstance())
    {
        juce::MessageManager::callAsync([this] { updateTexts(); });
    }
}

void PluginManagerContentComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    titleLabel.setText(lang.get("pluginManagerWindow.title"), juce::dontSendNotification);
    scanButton.setButtonText(lang.get("pluginManagerWindow.scan"));
    addButton.setButtonText(lang.get("pluginManagerWindow.add"));
    removeButton.setButtonText(lang.get("pluginManagerWindow.remove"));
    pluginListBox.updateContent();
}

void PluginManagerContentComponent::removeSelectedPlugin()
{
    int selectedRow = pluginListBox.getSelectedRow();
    if (selectedRow >= 0)
        getSharedPluginManager().removePluginFromListByIndex(selectedRow);
}

// --- PluginManagerWindow Implementation ---

PluginManagerWindow::PluginManagerWindow()
    : DocumentWindow(LanguageManager::getInstance().get("pluginManagerWindow.title"),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(400, 500, 800, 1000);
    setContentOwned(new PluginManagerContentComponent(), true);
    centreWithSize(500, 600);
    // The window is created invisible by default, the Application class will make it visible.
}

PluginManagerWindow::~PluginManagerWindow()
{
}

void PluginManagerWindow::closeButtonPressed()
{
    setVisible(false);
}