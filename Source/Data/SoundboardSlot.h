/*
  ==============================================================================

    SoundboardSlot.h
    Created: 18 Jun 2025 1:08:10pm
    Author:  Kevin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
    A data structure to hold the configuration for a single soundboard slot.
*/
struct SoundboardSlot
{
    SoundboardSlot() = default;

    SoundboardSlot(int id) 
        : slotId(id),
          displayName("Slot " + juce::String(id + 1))
    {
    }

    // The name to display on the button in the UI.
    juce::String displayName;

    // The absolute path to the audio file that has been copied to the app's data directory.
    juce::File audioFile;

    // The keyboard shortcut to trigger this sound.
    juce::KeyPress hotkey;

    // A unique identifier for this slot, typically from 0 to 8.
    int slotId = -1;

    /** Returns true if no audio file is associated with this slot. */
    bool isEmpty() const
    {
        return !audioFile.existsAsFile();
    }
};