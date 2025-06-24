#include "PluginManagementComponent.h"
#include "../../Application/Application.h"

PluginManagementComponent::PluginManagementComponent() {
    LanguageManager::getInstance().addChangeListener(this);

    addAndMakeVisible(scanPluginsButton);
    scanPluginsButton.onClick = [] {
        getSharedPluginManager().scanAndMergeAll();
        };

    // <<< NEW >>> Add the new button
    addAndMakeVisible(managePluginsButton);
    managePluginsButton.onClick = [] {
        if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
            app->showPluginManagerWindow();
        };

    addAndMakeVisible(addPluginButton);
    addPluginButton.onClick = [] {
        getSharedPluginManager().addPluginFromUserChoice();
        };

    updateTexts();
}

PluginManagementComponent::~PluginManagementComponent() {
    LanguageManager::getInstance().removeChangeListener(this);
}

void PluginManagementComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void PluginManagementComponent::resized() {
    auto bounds = getLocalBounds().reduced(15, 10);

    // <<< MODIFIED >>> Use FlexBox for easier layout management
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Add items in the desired order: Scan, Manage, Add
    fb.items.add(juce::FlexItem(scanPluginsButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10)); // Spacer
    fb.items.add(juce::FlexItem(managePluginsButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10)); // Spacer
    fb.items.add(juce::FlexItem(addPluginButton).withFlex(1.0f));

    fb.performLayout(bounds);
}

void PluginManagementComponent::updateTexts() {
    auto& lang = LanguageManager::getInstance();
    scanPluginsButton.setButtonText(lang.get("pluginManagement.scan"));
    managePluginsButton.setButtonText(lang.get("pluginManagement.manage")); // <<< NEW
    addPluginButton.setButtonText(lang.get("pluginManagement.add"));
}

void PluginManagementComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &LanguageManager::getInstance()) {
        updateTexts();
    }
}