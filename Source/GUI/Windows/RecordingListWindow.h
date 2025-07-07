#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include <functional>

class RecordingListContentComponent; // Forward Declaration

class RecordingListWindow : public juce::DocumentWindow
{
public:
    // Constructor nhận thêm thư mục con và callback
    RecordingListWindow(const juce::String& recordingsSubDir,
        std::function<void(const juce::File&)> onPlayCallback);
    ~RecordingListWindow() override;

    void closeButtonPressed() override;
    void refreshList(); // Hàm này để làm mới danh sách file

private:
    std::unique_ptr<RecordingListContentComponent> content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingListWindow)
};