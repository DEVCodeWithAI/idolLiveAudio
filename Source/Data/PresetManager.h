#pragma once

#include "JuceHeader.h"

// Forward declaration
class AudioEngine;

// <<< FIX >>> Define the identifiers in the header file to be included elsewhere.
namespace Identifiers
{
    const juce::Identifier Preset("Preset");
    const juce::Identifier VocalProcessorState("VocalProcessorState");
    const juce::Identifier MusicProcessorState("MusicProcessorState");
    const juce::Identifier MasterProcessorState("MasterProcessorState");
}

class PresetManager : public juce::ChangeBroadcaster
{
public:
    PresetManager();
    ~PresetManager();

    /** Saves the state of all processors from the AudioEngine into a file. */
    void savePreset(AudioEngine& engine, const juce::File& file);

    /** Loads the state of all processors into the AudioEngine from a file. */
    void loadPreset(AudioEngine& engine, const juce::File& file);

    void deletePreset(const juce::String& presetName);

    juce::File getPresetDirectory();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};