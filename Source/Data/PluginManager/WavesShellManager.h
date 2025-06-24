#pragma once

#include <JuceHeader.h>

/**
    A dedicated manager to find and safely scan WavesShell plugins.
    This isolates the potentially unstable Waves scan from the main plugin scan.
*/
class WavesShellManager
{
public:
    // Singleton access
    static WavesShellManager& getInstance();

    /** Scans the default VST3 locations for WaveShell files and parses them
        to get a list of individual Waves plugins. This operation is blocking.
    */
    void scanAndParseShells();

    /** Returns the list of Waves plugins found during the last successful scan.
    */
    const juce::Array<juce::PluginDescription>& getScannedPlugins() const;

private:
    // Private constructor/destructor for singleton
    WavesShellManager() = default;
    ~WavesShellManager() = default;

    /** Finds all WaveShell*.vst3 files in the default search paths. */
    void findWavesShellFiles(juce::Array<juce::File>& filesFound) const;

    juce::Array<juce::PluginDescription> knownWavesPlugins;
    juce::CriticalSection scannerLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavesShellManager)
};