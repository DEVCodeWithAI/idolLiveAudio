﻿#include "AppState.h"
#include "Data/LanguageManager/LanguageManager.h"
#include "../Data/PresetManager.h"
#include "../GUI/MainComponent/MainComponent.h"
#include "../GUI/Layout/TrackComponent.h"
#include "../GUI/Layout/MasterUtilityComponent.h"
#include "../GUI/Layout/PresetBarComponent.h"
#include "../GUI/Layout/MenubarComponent.h"
#include "juce_audio_devices/juce_audio_devices.h"


// Define identifiers for session state XML
namespace SessionIds
{
    const juce::Identifier SESSION("SESSION");
    const juce::Identifier DEVICEMANAGER("DEVICEMANAGER");
    const juce::Identifier CUSTOM_AUDIO_SETUP("CUSTOM_AUDIO_SETUP");
    const juce::Identifier ROUTING("ROUTING");
    const juce::Identifier VOCAL_INPUT("VOCAL_INPUT");
    const juce::Identifier MUSIC_INPUT("MUSIC_INPUT");
    const juce::Identifier APP_OUTPUT("APP_OUTPUT");
    const juce::Identifier ACTIVE_PRESET("ACTIVE_PRESET");
    const juce::Identifier presetName("name");
    const juce::Identifier OPEN_WINDOWS("OPEN_WINDOWS");
}

namespace WindowStateIds
{
    const juce::Identifier processorId("processorId");
}

AppState& AppState::getInstance()
{
    static AppState instance;
    return instance;
}

AppState::AppState()
{
    currentPresetName = LanguageManager::getInstance().get("presetbar.noPresetLoaded");
}

AppState::~AppState() {}

void AppState::setPresetDirty(bool newDirtyState)
{
    if (dirty != newDirtyState)
    {
        dirty = newDirtyState;
        sendChangeMessage();
    }
}

bool AppState::isPresetDirty() const
{
    return dirty;
}

void AppState::setCurrentPresetName(const juce::String& name)
{
    if (currentPresetName != name)
    {
        currentPresetName = name;
        sendChangeMessage();
    }
}

juce::String AppState::getCurrentPresetName() const
{
    return currentPresetName;
}

void AppState::markAsSaved(const juce::String& newName)
{
    setCurrentPresetName(newName);
    setPresetDirty(false);
}

juce::File AppState::getSessionFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName);

    if (!appDataDir.exists())
        appDataDir.createDirectory();

    return appDataDir.getChildFile("session.xml");
}

void AppState::saveState(MainComponent& mainComponent)
{
    auto& deviceManager = mainComponent.getAudioDeviceManager();
    auto& audioEngine = mainComponent.getAudioEngine();

    auto sessionXml = std::make_unique<juce::XmlElement>(SessionIds::SESSION);

    auto currentSetup = deviceManager.getAudioDeviceSetup();
    auto* deviceSetupXml = sessionXml->createNewChildElement(SessionIds::CUSTOM_AUDIO_SETUP);
    deviceSetupXml->setAttribute("deviceType", deviceManager.getCurrentAudioDeviceType());
    deviceSetupXml->setAttribute("inputDevice", currentSetup.inputDeviceName);
    deviceSetupXml->setAttribute("outputDevice", currentSetup.outputDeviceName);
    deviceSetupXml->setAttribute("sampleRate", currentSetup.sampleRate);
    deviceSetupXml->setAttribute("bufferSize", currentSetup.bufferSize);

    auto* routingStateXml = sessionXml->createNewChildElement(SessionIds::ROUTING);
    routingStateXml->setAttribute(SessionIds::VOCAL_INPUT, audioEngine.getVocalInputChannelName());
    routingStateXml->setAttribute(SessionIds::MUSIC_INPUT, audioEngine.getMusicInputChannelName());
    routingStateXml->setAttribute(SessionIds::APP_OUTPUT, audioEngine.getSelectedOutputChannelPairName());

    auto* presetStateXml = sessionXml->createNewChildElement(SessionIds::ACTIVE_PRESET);
    presetStateXml->setAttribute(SessionIds::presetName, getCurrentPresetName());

    auto* openWindowsStateXml = sessionXml->createNewChildElement(SessionIds::OPEN_WINDOWS);
    auto vocalWindowsState = mainComponent.getVocalTrack().getOpenWindowsState();
    if (vocalWindowsState.getNumChildren() > 0)
        openWindowsStateXml->addChildElement(vocalWindowsState.createXml().release());
    auto musicWindowsState = mainComponent.getMusicTrack().getOpenWindowsState();
    if (musicWindowsState.getNumChildren() > 0)
        openWindowsStateXml->addChildElement(musicWindowsState.createXml().release());
    auto masterWindowsState = mainComponent.getMasterUtilityComponent().getOpenWindowsState();
    if (masterWindowsState.getNumChildren() > 0)
        openWindowsStateXml->addChildElement(masterWindowsState.createXml().release());

    sessionXml->writeTo(getSessionFile());
}

bool AppState::loadAudioDeviceSetup(juce::AudioDeviceManager::AudioDeviceSetup& setupToFill, juce::String& loadedDeviceType)
{
    auto sessionFile = getSessionFile();
    if (!sessionFile.existsAsFile()) return false;

    if (auto xml = juce::parseXML(sessionFile))
    {
        auto* deviceSetupXml = xml->getChildByName(SessionIds::CUSTOM_AUDIO_SETUP);
        if (deviceSetupXml != nullptr)
        {
            loadedDeviceType = deviceSetupXml->getStringAttribute("deviceType");
            setupToFill.inputDeviceName = deviceSetupXml->getStringAttribute("inputDevice");
            setupToFill.outputDeviceName = deviceSetupXml->getStringAttribute("outputDevice");
            setupToFill.sampleRate = deviceSetupXml->getDoubleAttribute("sampleRate");
            setupToFill.bufferSize = deviceSetupXml->getIntAttribute("bufferSize");
            return true;
        }
    }
    return false;
}

void AppState::loadPostDeviceState(MainComponent& mainComponent)
{
    auto sessionFile = getSessionFile();
    if (!sessionFile.existsAsFile()) return;

    if (auto xml = juce::parseXML(sessionFile))
    {
        auto* routingStateXml = xml->getChildByName(SessionIds::ROUTING);
        if (routingStateXml != nullptr)
        {
            auto vocalInputName = routingStateXml->getStringAttribute(SessionIds::VOCAL_INPUT);
            auto musicInputName = routingStateXml->getStringAttribute(SessionIds::MUSIC_INPUT);
            auto appOutputName = routingStateXml->getStringAttribute(SessionIds::APP_OUTPUT);

            mainComponent.getAudioEngine().setVocalInputChannelByName(vocalInputName);
            mainComponent.getAudioEngine().setMusicInputChannelByName(musicInputName);
            mainComponent.getAudioEngine().setSelectedOutputChannelsByName(appOutputName);

            mainComponent.getVocalTrack().setSelectedInputChannelByName(vocalInputName);
            mainComponent.getMusicTrack().setSelectedInputChannelByName(musicInputName);
            if (auto* menubar = mainComponent.getMenubarComponent())
                menubar->setSelectedOutputChannelPairByName(appOutputName);
        }

        auto* presetStateXml = xml->getChildByName(SessionIds::ACTIVE_PRESET);
        if (presetStateXml != nullptr)
        {
            auto presetToLoad = presetStateXml->getStringAttribute(SessionIds::presetName);
            if (presetToLoad.isNotEmpty() && presetToLoad != LanguageManager::getInstance().get("presetbar.noPresetLoaded"))
            {
                mainComponent.getPresetBar().loadPresetByName(presetToLoad);

                auto* openWindowsXml = xml->getChildByName(SessionIds::OPEN_WINDOWS);
                if (openWindowsXml != nullptr)
                {
                    auto openWindowsState = juce::ValueTree::fromXml(*openWindowsXml);
                    if (openWindowsState.isValid())
                    {
                        juce::Timer::callAfterDelay(500, [&mainComponent, openWindowsState]() {
                            if (openWindowsState.isValid())
                            {
                                auto vocalId = Identifiers::VocalProcessorState.toString();
                                auto vocalWindows = openWindowsState.getChildWithProperty(WindowStateIds::processorId, vocalId);
                                if (vocalWindows.isValid())
                                    mainComponent.getVocalTrack().restoreOpenWindows(vocalWindows);

                                auto musicId = Identifiers::MusicProcessorState.toString();
                                auto musicWindows = openWindowsState.getChildWithProperty(WindowStateIds::processorId, musicId);
                                if (musicWindows.isValid())
                                    mainComponent.getMusicTrack().restoreOpenWindows(musicWindows);

                                auto masterId = Identifiers::MasterProcessorState.toString();
                                auto masterWindows = openWindowsState.getChildWithProperty(WindowStateIds::processorId, masterId);
                                if (masterWindows.isValid())
                                    mainComponent.getMasterUtilityComponent().restoreOpenWindows(masterWindows);
                            }
                            });
                    }
                }
            }
        }
    }
}