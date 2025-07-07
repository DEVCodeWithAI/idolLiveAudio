#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include <functional>

// Forward-declaration để tránh include vòng
class ProjectListContentComponent;

class ProjectListWindow : public juce::DocumentWindow
{
public:
    ProjectListWindow(const juce::String& recordingsSubDir,
        std::function<void(const juce::File&)> onPlayCallback);

    // Khai báo tường minh hàm hủy để sửa lỗi unique_ptr
    ~ProjectListWindow() override;

    void closeButtonPressed() override;
    void refreshList();

private:
    std::unique_ptr<ProjectListContentComponent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectListWindow)
};