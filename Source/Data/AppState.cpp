/*
  ==============================================================================

    AppState.cpp
    (Final version: Removed unused ACTIVE_PRESET logic and redundant function)

  ==============================================================================
*/
#include "AppState.h"
#include "Data/LanguageManager/LanguageManager.h"
#include "../Data/PresetManager.h"
#include "../GUI/MainComponent/MainComponent.h"
#include "../GUI/Layout/TrackComponent.h"
#include "../GUI/Layout/MasterUtilityComponent.h"
#include "../GUI/Layout/PresetBarComponent.h"
#include "../GUI/Layout/MenubarComponent.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include <juce_cryptography/juce_cryptography.h>


namespace SessionIds
{
    const juce::Identifier SESSION("SESSION");
    const juce::Identifier DEVICEMANAGER("DEVICEMANAGER");
    const juce::Identifier CUSTOM_AUDIO_SETUP("CUSTOM_AUDIO_SETUP");
    const juce::Identifier ROUTING("ROUTING");
    const juce::Identifier VOCAL_INPUT("VOCAL_INPUT");
    const juce::Identifier MUSIC_INPUT("MUSIC_INPUT");
    const juce::Identifier APP_OUTPUT("APP_OUTPUT");
    const juce::Identifier presetName("name");
    const juce::Identifier OPEN_WINDOWS("OPEN_WINDOWS");
    const juce::Identifier QUICK_PRESETS("QUICK_PRESETS");
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
    for (int i = 0; i < numQuickSlots; ++i)
        quickPresetSlots.add({});
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

    // Save Device Setup
    auto currentSetup = deviceManager.getAudioDeviceSetup();
    auto* deviceSetupXml = sessionXml->createNewChildElement(SessionIds::CUSTOM_AUDIO_SETUP);
    deviceSetupXml->setAttribute("deviceType", deviceManager.getCurrentAudioDeviceType());
    deviceSetupXml->setAttribute("inputDevice", currentSetup.inputDeviceName);
    deviceSetupXml->setAttribute("outputDevice", currentSetup.outputDeviceName);

    // Save Routing
    auto* routingStateXml = sessionXml->createNewChildElement(SessionIds::ROUTING);
    routingStateXml->setAttribute(SessionIds::VOCAL_INPUT, audioEngine.getVocalInputChannelName());
    routingStateXml->setAttribute(SessionIds::MUSIC_INPUT, audioEngine.getMusicInputChannelName());
    routingStateXml->setAttribute(SessionIds::APP_OUTPUT, audioEngine.getSelectedOutputChannelPairName());

    // Save Quick Preset Slots
    auto* quickPresetsXml = sessionXml->createNewChildElement(SessionIds::QUICK_PRESETS);
    for (int i = 0; i < quickPresetSlots.size(); ++i)
    {
        quickPresetsXml->setAttribute("slot" + juce::String(i), quickPresetSlots[i]);
    }

    // Save Open Windows
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

    // Write to file
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
        // Load Routing State
        auto* routingStateXml = xml->getChildByName(SessionIds::ROUTING);
        if (routingStateXml != nullptr)
        {
            auto vocalInputName = routingStateXml->getStringAttribute(SessionIds::VOCAL_INPUT);
            auto musicInputName = routingStateXml->getStringAttribute(SessionIds::MUSIC_INPUT);
            auto appOutputName = routingStateXml->getStringAttribute(SessionIds::APP_OUTPUT);

            mainComponent.getAudioEngine().setVocalInputChannelByName(vocalInputName);
            mainComponent.getAudioEngine().setMusicInputChannelByName(musicInputName);
            mainComponent.getAudioEngine().setSelectedOutputChannelsByName(appOutputName);

            if (auto* vocalSelector = mainComponent.getVocalTrack().getChannelSelector())
                vocalSelector->setSelectedChannelByName(vocalInputName);
            if (auto* musicSelector = mainComponent.getMusicTrack().getChannelSelector())
                musicSelector->setSelectedChannelByName(musicInputName);
            if (auto* menubar = mainComponent.getMenubarComponent())
                if (auto* outputSelector = menubar->getOutputSelector())
                    outputSelector->setSelectedChannelByName(appOutputName);
        }

        // Load Quick Preset Slots
        auto* quickPresetsXml = xml->getChildByName(SessionIds::QUICK_PRESETS);
        if (quickPresetsXml != nullptr)
        {
            for (int i = 0; i < quickPresetSlots.size(); ++i)
            {
                quickPresetSlots.set(i, quickPresetsXml->getStringAttribute("slot" + juce::String(i), {}));
            }
            sendChangeMessage();
        }
    }
}

void AppState::setSystemLocked(bool shouldBeLocked, const juce::String& password)
{
    systemLocked = shouldBeLocked;
    if (systemLocked && password.isNotEmpty())
        lockPasswordHash = juce::MD5(password.toRawUTF8(), password.getNumBytesAsUTF8()).toHexString();
    else if (!systemLocked)
        lockPasswordHash.clear();
    sendChangeMessage();
}

bool AppState::unlockSystem(const juce::String& passwordAttempt)
{
    if (juce::MD5(passwordAttempt.toRawUTF8(), passwordAttempt.getNumBytesAsUTF8()).toHexString() == lockPasswordHash)
    {
        setSystemLocked(false);
        return true;
    }
    return false;
}

bool AppState::isSystemLocked() const
{
    return systemLocked.load();
}

juce::String AppState::getPasswordHash() const
{
    return lockPasswordHash;
}

void AppState::loadLockState(bool isLocked, const juce::String& passwordHash)
{
    systemLocked = isLocked;
    lockPasswordHash = passwordHash;
    sendChangeMessage();
}

void AppState::assignQuickPreset(int slotIndex, const juce::String& presetName)
{
    if (juce::isPositiveAndBelow(slotIndex, quickPresetSlots.size()))
    {
        if (quickPresetSlots[slotIndex] != presetName)
        {
            quickPresetSlots.set(slotIndex, presetName);
            sendChangeMessage();
        }
    }
}

juce::String AppState::getQuickPresetName(int slotIndex) const
{
    if (juce::isPositiveAndBelow(slotIndex, quickPresetSlots.size()))
        return quickPresetSlots[slotIndex];
    return {};
}

int AppState::getNumQuickPresetSlots() const
{
    return numQuickSlots;
}

void AppState::setIsLoadingPreset(bool isLoading)
{
    loadingPresetFlag = isLoading;
}

bool AppState::isLoadingPreset() const
{
    return loadingPresetFlag.load();
}