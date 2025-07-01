#include "LanguageManager.h"

//==============================================================================
LanguageManager& LanguageManager::getInstance()
{
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager()
{
    loadLanguage("en"); // Mặc định tải tiếng Anh khi khởi động
}

LanguageManager::~LanguageManager()
{
}

//==============================================================================
void LanguageManager::loadLanguage(const juce::String& languageCode)
{
    juce::String resourceName;
    if (languageCode == "vi")
        resourceName = "lang_vi_json";
    else
        resourceName = "lang_en_json";

    int dataSize = 0;
    const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), dataSize);

    if (data != nullptr)
    {
        juce::var parsedJson;
        auto result = juce::JSON::parse(juce::String::fromUTF8(data, dataSize), parsedJson);

        if (result.wasOk())
        {
            languageData = parsedJson;
            sendChangeMessage(); // Thông báo cho các thành phần khác rằng ngôn ngữ đã thay đổi
        }
        else
        {
            DBG("LanguageManager - FAILED to parse JSON for language: " + languageCode);
        }
    }
    else
    {
        // <<< FIXED: Corrected the debug loop to use valid BinaryData members >>>
        DBG("LanguageManager - FAILED to find resource: " + resourceName);
        DBG("Please ensure the .json files are added to the Binary Resources in the Projucer.");
        DBG("Available resources are:");
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            DBG("  - " + juce::String(BinaryData::namedResourceList[i]));
        }
    }
}

juce::String LanguageManager::get(const juce::String& key)
{
    juce::StringArray keys;
    keys.addTokens(key, ".", "");

    juce::var currentVar = languageData;

    for (const auto& k : keys)
    {
        if (currentVar.isObject())
        {
            currentVar = currentVar.getProperty(juce::Identifier(k), juce::var());
        }
        else
        {
            return key;
        }
    }

    return currentVar.toString();
}