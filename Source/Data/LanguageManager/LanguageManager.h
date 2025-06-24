#pragma once

#include "JuceHeader.h"

class LanguageManager : public juce::ChangeBroadcaster
{
public:
    // Hàm để lấy thực thể duy nhất của lớp (Singleton)
    static LanguageManager& getInstance();

    // Tải một file ngôn ngữ từ thư mục Resources
    void loadLanguage(const juce::String& languageCode); // ví dụ: "en", "vi"

    // Lấy một chuỗi văn bản đã được dịch
    juce::String get(const juce::String& key);

private:
    // Constructor và Destructor là private để đảm bảo chỉ có một thực thể
    LanguageManager();
    ~LanguageManager();

    // Ngăn chặn việc sao chép
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LanguageManager)

    juce::var languageData; // Biến var để lưu trữ toàn bộ dữ liệu JSON
};
