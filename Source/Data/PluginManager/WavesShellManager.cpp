#include "WavesShellManager.h"

// Không cần include <windows.h> nữa, JUCE sẽ lo việc đó.

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
        DBG("No WavesShell files found to scan.");
        return;
    }

    DBG("Found " + juce::String(shellFiles.size()) + " WavesShell file(s). Starting isolated scan...");

    juce::VST3PluginFormat vst3Format;

    for (const auto& shellFile : shellFiles)
    {
        DBG("Scanning: " + shellFile.getFullPathName());
        juce::OwnedArray<juce::PluginDescription> descsInFile;

        try
        {
            vst3Format.findAllTypesForFile(descsInFile, shellFile.getFullPathName());
        }
        catch (const std::exception& e)
        {
            DBG("!!! C++ Exception while scanning: " + shellFile.getFullPathName() + " - " + juce::String(e.what()));
        }
        catch (...)
        {
            DBG("!!! Unknown C++ Exception while scanning: " + shellFile.getFullPathName());
        }

        if (descsInFile.size() > 0)
        {
            for (auto* desc : descsInFile)
            {
                if (desc != nullptr)
                    knownWavesPlugins.add(*desc);
            }
            DBG("Successfully parsed " + juce::String(descsInFile.size()) + " plugins from " + shellFile.getFileName());
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
                DBG("Ignoring potentially problematic Waves shell file: " + foundFile.getFullPathName());
            }
        }
    }
}