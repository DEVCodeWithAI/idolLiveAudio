#include "PresetManager.h"
#include "../AudioEngine/AudioEngine.h"
#include "../Data/AppState.h"

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

// <<< MODIFIED: Now saves the state of all 8 FX processors >>>
void PresetManager::savePreset(AudioEngine& engine, const juce::File& file)
{
    juce::ValueTree presetState(Identifiers::Preset);
    auto& appState = AppState::getInstance();

    // --- LƯU TRẠNG THÁI KHÓA ---
    // Lấy trạng thái khóa và mật khẩu đã hash từ AppState
    bool isLocked = appState.isSystemLocked();
    juce::String passwordHash = appState.getPasswordHash(); // Cần thêm hàm này vào AppState

    // Đặt các thuộc tính vào ValueTree
    presetState.setProperty(Identifiers::lockState, isLocked, nullptr);
    if (isLocked)
    {
        presetState.setProperty(Identifiers::lockPasswordHash, passwordHash, nullptr);
    }

    // --- LƯU CÁC PROCESSOR ---
    presetState.addChild(engine.getVocalProcessor().getState(), -1, nullptr);
    presetState.addChild(engine.getMusicProcessor().getState(), -1, nullptr);
    presetState.addChild(engine.getMasterProcessor().getState(), -1, nullptr);

    // Lưu các FX processor
    for (int i = 0; i < 4; ++i)
    {
        if (auto* vocalFx = engine.getFxProcessorForVocal(i))
            presetState.addChild(vocalFx->getState(), -1, nullptr);
        if (auto* musicFx = engine.getFxProcessorForMusic(i))
            presetState.addChild(musicFx->getState(), -1, nullptr);
    }

    // Ghi ValueTree ra file XML
    if (auto xml = std::unique_ptr<juce::XmlElement>(presetState.createXml()))
    {
        xml->writeTo(file, {});
    }
}

// <<< MODIFIED: Now loads the state of all 8 FX processors >>>
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
            auto& appState = AppState::getInstance();

            // --- NẠP TRẠNG THÁI KHÓA ---
            // Đọc trạng thái và hash từ file preset
            bool isLocked = presetState.getProperty(Identifiers::lockState, false);
            juce::String passwordHash = presetState.getProperty(Identifiers::lockPasswordHash, "");

            // Cập nhật trạng thái khóa vào AppState
            appState.loadLockState(isLocked, passwordHash); // Cần thêm hàm này vào AppState


            // --- NẠP CÁC PROCESSOR ---
            auto vocalState = presetState.getChildWithName(Identifiers::VocalProcessorState);
            if (vocalState.isValid())
                engine.getVocalProcessor().setState(vocalState);

            auto musicState = presetState.getChildWithName(Identifiers::MusicProcessorState);
            if (musicState.isValid())
                engine.getMusicProcessor().setState(musicState);

            auto masterState = presetState.getChildWithName(Identifiers::MasterProcessorState);
            if (masterState.isValid())
                engine.getMasterProcessor().setState(masterState);

            // Nạp các FX processor
            const juce::Identifier fxIds[] = {
                Identifiers::VocalFx1State, Identifiers::VocalFx2State, Identifiers::VocalFx3State, Identifiers::VocalFx4State,
                Identifiers::MusicFx1State, Identifiers::MusicFx2State, Identifiers::MusicFx3State, Identifiers::MusicFx4State
            };

            for (int i = 0; i < 4; ++i)
            {
                auto vocalFxState = presetState.getChildWithName(fxIds[i]);
                if (vocalFxState.isValid())
                    if (auto* proc = engine.getFxProcessorForVocal(i))
                        proc->setState(vocalFxState);

                auto musicFxState = presetState.getChildWithName(fxIds[i + 4]);
                if (musicFxState.isValid())
                    if (auto* proc = engine.getFxProcessorForMusic(i))
                        proc->setState(musicFxState);
            }
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