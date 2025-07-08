#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"

class AudioEngine;
class ProjectListWindow;

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
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    void updateTexts();
    void recordProject();
    void stopProjectRecording();
    void manageProjects();
    void playProject();
    void stopProject();

    AudioEngine& audioEngine;

    // <<< SỬA: Thay thế WeakReference bằng Component::SafePointer >>>
    juce::Component::SafePointer<ProjectListWindow> projectListWindow;

    juce::Label loadedProjectTitleLabel;
    juce::Label loadedProjectLabel;
    juce::TextButton recordProjectButton;
    juce::TextButton manageProjectsButton;
    juce::TextButton playProjectButton;
    juce::TextButton stopProjectButton;
};