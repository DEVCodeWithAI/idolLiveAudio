#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include <functional>

class ProjectListContentComponent;
class AudioEngine;

class ProjectListWindow : public juce::DocumentWindow
{
public:
    ProjectListWindow(const juce::String& recordingsSubDir,
        AudioEngine& engine,
        std::function<void(const juce::File&)> onPlayCallback);

    ~ProjectListWindow() override;

    void closeButtonPressed() override;
    void refreshList();

    // <<< XÓA: Dòng callback này không còn cần thiết >>>
    // std::function<void()> onWindowBeingDeleted;

private:
    std::unique_ptr<ProjectListContentComponent> content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectListWindow)
};