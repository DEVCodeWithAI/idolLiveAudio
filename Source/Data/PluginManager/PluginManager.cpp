// Source/Data/PluginManager/PluginManager.cpp

#include "Data/PluginManager/PluginManager.h"
#include "Data/PluginManager/WavesShellManager.h"

PluginManager::PluginManager()
{
    formatManager.addDefaultFormats();

    knownPluginsFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("knownPlugins.xml");

    loadKnownPluginsList();
}

PluginManager::~PluginManager()
{
    DBG("PluginManager destroyed cleanly.");
}

void PluginManager::scanAndMergeAll()
{
    knownPluginList.clear();
    juce::VST3PluginFormat vst3Format;

    // --- NEW, ROBUST SCANNING LOGIC ---

    // 1. Manually find all VST3 plugin files from default locations.
    juce::Array<juce::File> allPluginFiles;

    // <<< FIX IS HERE: Correctly iterate over juce::FileSearchPath >>>
    auto searchPaths = vst3Format.getDefaultLocationsToSearch();
    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        const juce::File path = searchPaths[i];
        path.findChildFiles(allPluginFiles, juce::File::findFiles, true, "*.vst3");
    }

    // 2. Scan non-Waves plugins first, skipping any WaveShell files.
    DBG("Starting standard VST3 plugin scan (excluding WaveShells)...");
    for (const auto& pluginFile : allPluginFiles)
    {
        if (pluginFile.getFileName().containsIgnoreCase("WaveShell"))
        {
            DBG("Skipping WaveShell file, will be handled by WavesShellManager: " + pluginFile.getFileName());
            continue;
        }

        // Add a try-catch block for safety against other misbehaving plugins.
        try
        {
            juce::OwnedArray<juce::PluginDescription> descs;
            vst3Format.findAllTypesForFile(descs, pluginFile.getFullPathName());
            for (auto* desc : descs)
                if (desc != nullptr)
                    knownPluginList.addType(*desc);
        }
        catch (...)
        {
            DBG("An exception was caught while scanning a non-Waves plugin: " + pluginFile.getFileName() + ". Skipping this file.");
        }
    }
    DBG("Standard scan complete. Found " + juce::String(knownPluginList.getNumTypes()) + " standard plugins.");

    // 3. Now, handle Waves plugins using the dedicated, safe manager.
    DBG("Starting isolated WavesShell scan...");
    WavesShellManager::getInstance().scanAndParseShells();
    const auto& wavesPlugins = WavesShellManager::getInstance().getScannedPlugins();
    DBG("Found " + juce::String(wavesPlugins.size()) + " Waves plugins.");
    for (const auto& wavesDesc : wavesPlugins)
    {
        knownPluginList.addType(wavesDesc);
    }

    saveKnownPluginsList();
    sendChangeMessage();
    DBG("Full plugin scan and merge complete.");
}

void PluginManager::addPluginFromUserChoice()
{
    auto fc = std::make_shared<juce::FileChooser>("Add VST3 Plugin", juce::File{}, "*.vst3");

    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, fc](const juce::FileChooser& chooser)
        {
            const juce::File pluginFile = chooser.getResult();
            if (pluginFile == juce::File{})
                return;

            if (pluginFile.getFileName().containsIgnoreCase("WaveShell"))
            {
                // If user selects a waveshell, trigger a full rescan to handle it correctly
                scanAndMergeAll();
                return;
            }

            bool alreadyInList = false;
            for (const auto& desc : knownPluginList.getTypes())
            {
                if (desc.fileOrIdentifier == pluginFile.getFullPathName())
                {
                    alreadyInList = true;
                    break;
                }
            }

            if (alreadyInList)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Plugin Not Added",
                    "The selected plugin is already in the list.");
                return;
            }

            juce::VST3PluginFormat vst3Format;
            juce::OwnedArray<juce::PluginDescription> descs;
            vst3Format.findAllTypesForFile(descs, pluginFile.getFullPathName());

            if (descs.size() > 0)
            {
                for (auto* desc : descs)
                    knownPluginList.addType(*desc);

                saveKnownPluginsList();
                sendChangeMessage();
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Plugin Scan Failed",
                    "Could not find any valid VST3 plugins in the selected file.");
            }
        });
}

void PluginManager::removePluginFromListByIndex(int index)
{
    if (isPositiveAndBelow(index, knownPluginList.getTypes().size()))
    {
        auto description = knownPluginList.getTypes()[index];
        knownPluginList.removeType(description);
        saveKnownPluginsList();
        sendChangeMessage();
    }
}

juce::Array<juce::PluginDescription> PluginManager::getKnownPlugins() const
{
    return knownPluginList.getTypes();
}

juce::Array<juce::PluginDescription> PluginManager::getFilteredPlugins(const juce::String& searchText) const
{
    const auto& allPlugins = getKnownPlugins();

    if (searchText.isEmpty())
        return allPlugins;

    juce::Array<juce::PluginDescription> filteredList;

    for (const auto& desc : allPlugins)
    {
        if (desc.name.containsIgnoreCase(searchText))
            filteredList.add(desc);
    }

    return filteredList;
}

std::unique_ptr<juce::AudioPluginInstance> PluginManager::createPluginInstance(const juce::PluginDescription& description,
    const juce::dsp::ProcessSpec& spec)
{
    juce::String errorMessage;
    auto instance = formatManager.createPluginInstance(description, spec.sampleRate, (int)spec.maximumBlockSize, errorMessage);

    if (instance == nullptr)
    {
        DBG("Failed to create plugin instance: " + errorMessage);
        return nullptr;
    }

    return instance;
}

void PluginManager::saveKnownPluginsList()
{
    if (auto xml = knownPluginList.createXml())
        xml->writeTo(knownPluginsFile, {});
}

void PluginManager::loadKnownPluginsList()
{
    if (knownPluginsFile.existsAsFile())
        if (auto xml = juce::parseXML(knownPluginsFile))
            knownPluginList.recreateFromXml(*xml);
}

void PluginManager::replaceAllPlugins(const juce::KnownPluginList& newList)
{
    knownPluginList.clear();
    for (const auto& desc : newList.getTypes())
    {
        knownPluginList.addType(desc);
    }
    saveKnownPluginsList();
    sendChangeMessage();
    DBG("Plugin list updated from non-blocking scan.");
}