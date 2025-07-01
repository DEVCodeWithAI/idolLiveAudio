#include "WavesShellManager.h"

WavesShellManager& WavesShellManager::getInstance()
{
    static WavesShellManager instance;
    return instance;
}

void WavesShellManager::scanAndParseShells()
{
    const juce::ScopedLock sl(scannerLock);

    knownWavesPlugins.clear();
    juce::Array<juce::File> shellFiles;
    findWavesShellFiles(shellFiles);

    if (shellFiles.isEmpty())
    {
        return;
    }

    juce::VST3PluginFormat vst3Format;

    for (const auto& shellFile : shellFiles)
    {
        juce::OwnedArray<juce::PluginDescription> descsInFile;

        try
        {
            vst3Format.findAllTypesForFile(descsInFile, shellFile.getFullPathName());
        }
        catch (const std::exception&)
        {
            // Exception caught, but we will not log it in the release version.
        }
        catch (...)
        {
            // Unknown exception caught.
        }

        if (descsInFile.size() > 0)
        {
            for (auto* desc : descsInFile)
            {
                if (desc != nullptr)
                    knownWavesPlugins.add(*desc);
            }
        }
    }
}

const juce::Array<juce::PluginDescription>& WavesShellManager::getScannedPlugins() const
{
    return knownWavesPlugins;
}

void WavesShellManager::findWavesShellFiles(juce::Array<juce::File>& filesFound) const
{
    auto searchPaths = juce::VST3PluginFormat().getDefaultLocationsToSearch();

    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        const juce::File path = searchPaths[i];
        juce::Array<juce::File> results;
        path.findChildFiles(results, juce::File::findFiles, false, "WaveShell*.vst3");

        for (const auto& foundFile : results)
        {
            if (!foundFile.getFileName().containsIgnoreCase("InternalSynth"))
            {
                filesFound.add(foundFile);
            }
            else
            {
                // Ignoring potentially problematic Waves shell file.
            }
        }
    }
}