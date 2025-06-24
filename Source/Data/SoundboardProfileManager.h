/*
  ==============================================================================

    SoundboardProfileManager.h
    Created: 18 Jun 2025 1:25:10pm
    Author:  Kevin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SoundboardSlot.h"

/**
    Manages the loading, saving, and state of soundboard profiles.
    This class is a ChangeBroadcaster, so UI components can listen for when a
    profile is loaded or changed.
*/
class SoundboardProfileManager : public juce::ChangeBroadcaster
{
public:
    SoundboardProfileManager();
    ~SoundboardProfileManager() override;

    /** Saves the provided array of slots as the current profile. */
    void saveProfile(const juce::Array<SoundboardSlot>& slots);

    /** Loads the default profile from disk if it exists. */
    void loadProfile();

    /** Clears the current profile and deletes associated files. (To be implemented) */
    void cleanProfile();

    /** Imports a profile from a .zip file. (To be implemented) */
    void importProfile(const juce::File& zipFile);

    /** Exports the current profile to a .zip file. (To be implemented) */
    void exportProfile(const juce::File& destinationZipFile);

    /** Returns the currently active array of 9 soundboard slots. */
    const juce::Array<SoundboardSlot>& getCurrentSlots() const;
    juce::File getProfileDirectory() const;

private:

    /** Gets the file path for the default profile JSON file. */
    juce::File getProfileJsonFile() const;
    
    // Helper functions for JSON conversion
    juce::var soundboardSlotToVar(const SoundboardSlot& slot);
    SoundboardSlot varToSoundboardSlot(const juce::var& slotVar);

    juce::Array<SoundboardSlot> currentSlots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundboardProfileManager)
};