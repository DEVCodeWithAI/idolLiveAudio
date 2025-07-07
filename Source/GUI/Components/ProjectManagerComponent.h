#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"

class AudioEngine;
class ProjectListWindow;

// <<< SỬA: Kế thừa từ cả hai listener >>>
class ProjectManagerComponent : public juce::Component,
    private juce::ChangeListener,
    private juce::ValueTree::Listener
{
public:
    ProjectManagerComponent(AudioEngine& engine);
    ~ProjectManagerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Hàm callback cho ChangeListener (đã có)
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // <<< THÊM: Hàm callback cho ValueTree::Listener >>>
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void updateTexts();

    void recordProject();
    void stopProjectRecording();
    void manageProjects();
    void playProject();
    void stopProject();

    AudioEngine& audioEngine;

    std::unique_ptr<ProjectListWindow> projectListWindow;

    // <<< THÊM: Label để hiển thị tên project đã load >>>
    juce::Label loadedProjectTitleLabel;
    juce::Label loadedProjectLabel;

    juce::TextButton recordProjectButton;
    juce::TextButton manageProjectsButton;
    juce::TextButton playProjectButton;
    juce::TextButton stopProjectButton;
};