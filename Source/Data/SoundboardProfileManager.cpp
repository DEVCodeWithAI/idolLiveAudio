/*
  ==============================================================================

    SoundboardProfileManager.cpp
    (Fixed ZipInputStream memory leak)

  ==============================================================================
*/

#include "SoundboardProfileManager.h"

SoundboardProfileManager::SoundboardProfileManager()
{
    // Initialize with 9 empty slots
    for (int i = 0; i < 9; ++i)
    {
        currentSlots.add(SoundboardSlot(i));
    }

    // Attempt to load the last saved profile on startup
    loadProfile();
}

SoundboardProfileManager::~SoundboardProfileManager()
{
}

void SoundboardProfileManager::saveProfile(const juce::Array<SoundboardSlot>& slots)
{
    juce::Array<juce::var> slotJsonArray;
    for (const auto& slot : slots)
    {
        slotJsonArray.add(soundboardSlotToVar(slot));
    }

    auto profileFile = getProfileJsonFile();
    profileFile.replaceWithData(juce::JSON::toString(slotJsonArray).toRawUTF8(),
        juce::JSON::toString(slotJsonArray).length());

    currentSlots = slots;
    sendChangeMessage(); // Notify listeners that the profile has changed
}

void SoundboardProfileManager::loadProfile()
{
    auto profileFile = getProfileJsonFile();
    if (!profileFile.existsAsFile())
        return;

    auto parsedJson = juce::JSON::parse(profileFile);

    if (!parsedJson.isUndefined() && parsedJson.isArray())
    {
        juce::Array<SoundboardSlot> loadedSlots;
        for (const auto& slotVar : *parsedJson.getArray())
        {
            loadedSlots.add(varToSoundboardSlot(slotVar));
        }

        if (loadedSlots.size() == 9)
        {
            currentSlots = loadedSlots;
            sendChangeMessage(); // Notify listeners
        }
    }
}

void SoundboardProfileManager::cleanProfile()
{
    auto profileDir = getProfileDirectory();

    for (const auto& slot : currentSlots)
    {
        if (!slot.isEmpty() && slot.audioFile.isAChildOf(profileDir))
        {
            slot.audioFile.deleteFile();
        }
    }

    getProfileJsonFile().deleteFile();

    currentSlots.clear();
    for (int i = 0; i < 9; ++i)
    {
        currentSlots.add(SoundboardSlot(i));
    }

    sendChangeMessage();
    DBG("Soundboard profile cleaned.");
}

// <<< FIX: Use std::unique_ptr to manage the ZipInputStream lifetime >>>
void SoundboardProfileManager::importProfile(const juce::File& zipFile)
{
    if (!zipFile.existsAsFile() || zipFile.getFileExtension() != ".zip")
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Import Failed", "Please select a valid .zip profile file.");
        return;
    }

    cleanProfile();

    juce::ZipFile zip(zipFile);
    bool success = true;

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        if (auto* entry = zip.getEntry(i))
        {
            juce::File targetFile = getProfileDirectory().getChildFile(entry->filename);

            // Use a std::unique_ptr to take ownership of the returned stream.
            // When 'inputStream' goes out of scope at the end of the 'if' block,
            // the ZipInputStream will be automatically deleted.
            if (const std::unique_ptr<juce::InputStream> inputStream{ zip.createStreamForEntry(*entry) })
            {
                juce::FileOutputStream outputStream(targetFile);
                if (outputStream.openedOk())
                {
                    outputStream.writeFromInputStream(*inputStream, -1);
                }
                else
                {
                    success = false;
                    break;
                }
            }
        }
    }

    if (success)
    {
        loadProfile();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Import Successful", "The soundboard profile has been imported.");
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Import Failed", "Could not extract files from the profile archive.");
    }
}


void SoundboardProfileManager::exportProfile(const juce::File& destinationZipFile)
{
    if (destinationZipFile.exists())
        destinationZipFile.deleteFile();

    juce::ZipFile::Builder zipBuilder;
    auto profileJson = getProfileJsonFile();

    if (profileJson.existsAsFile())
        zipBuilder.addFile(profileJson, 9, "default_profile.json");

    for (const auto& slot : currentSlots)
    {
        if (!slot.isEmpty() && slot.audioFile.existsAsFile())
        {
            zipBuilder.addFile(slot.audioFile, 9, slot.audioFile.getFileName());
        }
    }

    juce::FileOutputStream stream(destinationZipFile);
    if (stream.openedOk())
    {
        zipBuilder.writeToStream(stream, nullptr);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Export Successful", "The soundboard profile has been exported to:\n" + destinationZipFile.getFullPathName());
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Export Failed", "Could not write to the destination file.");
    }
}


const juce::Array<SoundboardSlot>& SoundboardProfileManager::getCurrentSlots() const
{
    return currentSlots;
}

juce::File SoundboardProfileManager::getProfileDirectory() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("SoundboardProfiles");

    if (!dir.exists())
        dir.createDirectory();

    return dir;
}

juce::File SoundboardProfileManager::getProfileJsonFile() const
{
    return getProfileDirectory().getChildFile("default_profile.json");
}

juce::var SoundboardProfileManager::soundboardSlotToVar(const SoundboardSlot& slot)
{
    auto slotObject = new juce::DynamicObject();
    slotObject->setProperty("slotId", slot.slotId);
    slotObject->setProperty("displayName", slot.displayName);
    slotObject->setProperty("audioFilePath", slot.audioFile.getFullPathName());
    slotObject->setProperty("hotkeyCode", slot.hotkey.getKeyCode());
    slotObject->setProperty("hotkeyModifiers", slot.hotkey.getModifiers().getRawFlags());
    return juce::var(slotObject);
}

SoundboardSlot SoundboardProfileManager::varToSoundboardSlot(const juce::var& slotVar)
{
    SoundboardSlot slot;
    if (auto* obj = slotVar.getDynamicObject())
    {
        slot.slotId = obj->getProperty("slotId");
        slot.displayName = obj->getProperty("displayName");
        slot.audioFile = juce::File(obj->getProperty("audioFilePath").toString());

        int keyCode = obj->getProperty("hotkeyCode");
        int modifiers = obj->getProperty("hotkeyModifiers");
        slot.hotkey = juce::KeyPress(keyCode, juce::ModifierKeys(modifiers), 0);
    }
    return slot;
}