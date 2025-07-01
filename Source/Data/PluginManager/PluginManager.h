#pragma once

#include <JuceHeader.h>
#include "WavesShellManager.h" 

class PluginManager : public juce::ChangeBroadcaster
{
public:
    PluginManager();
    ~PluginManager() override;

    void scanAndMergeAll();

    juce::Array<juce::PluginDescription> getFilteredPlugins(const juce::String& searchText) const;
    void replaceAllPlugins(const juce::KnownPluginList& newList);
    void addPluginFromUserChoice();
    void removePluginFromListByIndex(int index);
    juce::Array<juce::PluginDescription> getKnownPlugins() const;
    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(const juce::PluginDescription& description,
        const juce::dsp::ProcessSpec& spec);

private:
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void saveKnownPluginsList();
    void loadKnownPluginsList();

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    juce::File knownPluginsFile;
};