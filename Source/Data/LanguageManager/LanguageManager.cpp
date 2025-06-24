#include "LanguageManager.h"

//==============================================================================
LanguageManager& LanguageManager::getInstance()
{
    // Tạo một thực thể tĩnh duy nhất
    static LanguageManager instance;
    return instance;
}

LanguageManager::LanguageManager()
{
    // Mặc định tải tiếng Anh khi khởi động
    loadLanguage("en");
}

LanguageManager::~LanguageManager()
{
}

//==============================================================================
void LanguageManager::loadLanguage(const juce::String& languageCode)
{
    // Tìm file ngôn ngữ trong BinaryData được tạo ra từ thư mục Resources
    int dataSize = 0;
    const char* data = nullptr;

    if (languageCode == "vi")
        data = BinaryData::getNamedResource("lang_vi_json", dataSize);
    else // Mặc định là tiếng Anh
        data = BinaryData::getNamedResource("lang_en_json", dataSize);

    if (data != nullptr)
    {
        // Phân tích chuỗi JSON thành đối tượng juce::var
        juce::JSON::parse(juce::String::fromUTF8(data, dataSize), languageData);

        // Thông báo cho các thành phần khác rằng ngôn ngữ đã thay đổi
        sendChangeMessage();
    }
    else
    {
        // Nếu không tìm thấy file, báo lỗi
        DBG("Could not load language file for code: " + languageCode);
    }
}

juce::String LanguageManager::get(const juce::String& key)
{
    // Key có dạng "group.subgroup.key"
    // Ví dụ: "menubar.audioDevice"
    juce::StringArray keys;
    keys.addTokens(key, ".", "");

    juce::var currentVar = languageData;

    // Duyệt qua cây JSON để tìm giá trị
    for (const auto& k : keys)
    {
        if (currentVar.isObject())
        {
            currentVar = currentVar.getProperty(juce::Identifier(k), juce::var());
        }
        else
        {
            // Nếu không tìm thấy key, trả về key gốc để dễ dàng debug
            return key;
        }
    }

    // Trả về giá trị dưới dạng String
    return currentVar.toString();
}
