#pragma once

#include "JuceHeader.h"

// Forward declaration
class AudioEngine;

// <<< FIXED: Identifiers are now correctly defined back in this central header >>>
namespace Identifiers
{
    const juce::Identifier Preset("Preset");
    const juce::Identifier VocalProcessorState("VocalProcessorState");
    const juce::Identifier MusicProcessorState("MusicProcessorState");
    const juce::Identifier MasterProcessorState("MasterProcessorState");

    const juce::Identifier VocalFx1State("VocalFx1State");
    const juce::Identifier VocalFx2State("VocalFx2State");
    const juce::Identifier VocalFx3State("VocalFx3State");
    const juce::Identifier VocalFx4State("VocalFx4State");

    const juce::Identifier MusicFx1State("MusicFx1State");
    const juce::Identifier MusicFx2State("MusicFx2State");
    const juce::Identifier MusicFx3State("MusicFx3State");
    const juce::Identifier MusicFx4State("MusicFx4State");

    const juce::Identifier lockState("lockState");
    const juce::Identifier lockPasswordHash("lockPasswordHash");
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