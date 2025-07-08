#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"
#include <functional>

class ProjectListContentComponent;
class AudioEngine;

class ProjectListWindow : public juce::DocumentWindow
{
public:
    // <<< MODIFIED: Constructor now accepts a close callback >>>
    ProjectListWindow(const juce::String& recordingsSubDir,
        AudioEngine& engine,
        std::function<void(const juce::File&)> onProjectChosen,
        std::function<void()> onWindowClosed);

    ~ProjectListWindow() override;

    void closeButtonPressed() override;
    void refreshList();


private:
    // <<< ADDED: Member to store the close callback >>>
    std::function<void()> onWindowClosedCallback;
    std::unique_ptr<ProjectListContentComponent> content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectListWindow)
};