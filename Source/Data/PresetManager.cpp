#include "PresetManager.h"
#include "../AudioEngine/AudioEngine.h"

// <<< FIX >>> The Identifiers namespace has been moved to PresetManager.h
// It is no longer defined here.

PresetManager::PresetManager() {}
PresetManager::~PresetManager() {}

juce::File PresetManager::getPresetDirectory()
{
    auto userDocs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto presetDir = userDocs.getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("Presets");

    if (!presetDir.exists())
        presetDir.createDirectory();

    return presetDir;
}

void PresetManager::savePreset(AudioEngine& engine, const juce::File& file)
{
    juce::ValueTree presetState(Identifiers::Preset);

    presetState.addChild(engine.getVocalProcessor().getState(), -1, nullptr);
    presetState.addChild(engine.getMusicProcessor().getState(), -1, nullptr);
    presetState.addChild(engine.getMasterProcessor().getState(), -1, nullptr);

    if (auto xml = std::unique_ptr<juce::XmlElement>(presetState.createXml()))
    {
        xml->writeTo(file, {});
    }
}

void PresetManager::loadPreset(AudioEngine& engine, const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("PresetManager::loadPreset - File does not exist: " + file.getFullPathName());
        return;
    }

    juce::XmlDocument xmlDoc(file);
    if (auto xml = std::unique_ptr<juce::XmlElement>(xmlDoc.getDocumentElement()))
    {
        auto presetState = juce::ValueTree::fromXml(*xml);

        if (presetState.hasType(Identifiers::Preset))
        {
            auto vocalState = presetState.getChildWithName(Identifiers::VocalProcessorState);
            if (vocalState.isValid())
                engine.getVocalProcessor().setState(vocalState);

            auto musicState = presetState.getChildWithName(Identifiers::MusicProcessorState);
            if (musicState.isValid())
                engine.getMusicProcessor().setState(musicState);

            auto masterState = presetState.getChildWithName(Identifiers::MasterProcessorState);
            if (masterState.isValid())
                engine.getMasterProcessor().setState(masterState);
        }
    }
    else
    {
        DBG("PresetManager::loadPreset - Could not parse XML from file: " + file.getFullPathName());
    }
}

void PresetManager::deletePreset(const juce::String& presetName)
{
    auto presetFile = getPresetDirectory().getChildFile(presetName + ".xml");
    if (presetFile.existsAsFile())
    {
        if (presetFile.deleteFile())
        {
            // Notify listeners that the list of presets has changed
            sendChangeMessage();
        }
    }
}