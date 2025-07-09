#include "ProjectManagerComponent.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../Windows/ProjectListWindow.h"

// Thêm namespace cho các key của ValueTree
namespace ProjectStateIDs
{
    const juce::Identifier name("name");
    const juce::Identifier isPlaying("isPlaying");
}

ProjectManagerComponent::ProjectManagerComponent(AudioEngine& engine) : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    audioEngine.getProjectState().addListener(this);

    addAndMakeVisible(recordProjectButton);
    recordProjectButton.setClickingTogglesState(true);
    recordProjectButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    recordProjectButton.onClick = [this] {
        if (recordProjectButton.getToggleState())
            recordProject();
        else
            stopProjectRecording();
        };

    addAndMakeVisible(manageProjectsButton);
    manageProjectsButton.onClick = [this] { manageProjects(); };
    addAndMakeVisible(playProjectButton);
    playProjectButton.onClick = [this] { playProject(); };
    addAndMakeVisible(stopProjectButton);
    stopProjectButton.onClick = [this] { stopProject(); };

    addAndMakeVisible(loadedProjectTitleLabel);
    loadedProjectTitleLabel.setFont(IdolUIHelpers::createRegularFont(14.0f));
    loadedProjectTitleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    loadedProjectTitleLabel.setJustificationType(juce::Justification::bottomLeft);

    addAndMakeVisible(loadedProjectLabel);
    loadedProjectLabel.setFont(IdolUIHelpers::createBoldFont(18.0f));
    loadedProjectLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    loadedProjectLabel.setJustificationType(juce::Justification::topLeft);

    valueTreePropertyChanged(audioEngine.getProjectState(), {});
    updateTexts();
}

ProjectManagerComponent::~ProjectManagerComponent()
{
    // unique_ptr handles the destruction automatically
    LanguageManager::getInstance().removeChangeListener(this);
    audioEngine.getProjectState().removeListener(this);
}

void ProjectManagerComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
}

void ProjectManagerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto rightArea = bounds.removeFromRight(450);
    auto leftArea = bounds;

    loadedProjectTitleLabel.setBounds(leftArea.removeFromTop(leftArea.getHeight() / 2));
    loadedProjectLabel.setBounds(leftArea);

    juce::FlexBox fb;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    fb.alignItems = juce::FlexBox::AlignItems::stretch;

    const float gap = 10.0f;

    fb.items.add(juce::FlexItem(recordProjectButton).withFlex(1.3f));
    fb.items.add(juce::FlexItem(playProjectButton).withFlex(1.0f).withMargin({ 0, 0, 0, gap }));
    fb.items.add(juce::FlexItem(stopProjectButton).withFlex(1.0f).withMargin({ 0, 0, 0, gap }));
    fb.items.add(juce::FlexItem(manageProjectsButton).withFlex(1.3f).withMargin({ 0, 0, 0, gap }));

    fb.performLayout(rightArea);
}

void ProjectManagerComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
}

void ProjectManagerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    if (tree.hasType("ProjectState"))
    {
        auto& lang = LanguageManager::getInstance();
        bool isPlaying = tree.getProperty(ProjectStateIDs::isPlaying);
        juce::String projectName = tree.getProperty(ProjectStateIDs::name).toString();

        loadedProjectTitleLabel.setText(lang.get("projectManager.loaded"), juce::dontSendNotification);

        if (projectName.isNotEmpty())
        {
            loadedProjectLabel.setText(projectName, juce::dontSendNotification);
            playProjectButton.setEnabled(!isPlaying);
            stopProjectButton.setEnabled(isPlaying);
            recordProjectButton.setEnabled(false);
        }
        else
        {
            loadedProjectLabel.setText(lang.get("projectManager.noProjectLoaded"), juce::dontSendNotification);
            playProjectButton.setEnabled(false);
            stopProjectButton.setEnabled(false);
            recordProjectButton.setEnabled(true);
        }
    }
}

void ProjectManagerComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    if (recordProjectButton.getToggleState())
        recordProjectButton.setButtonText(juce::String::fromUTF8("● ") + lang.get("projectManager.recording"));
    else
        recordProjectButton.setButtonText(lang.get("projectManager.record"));

    manageProjectsButton.setButtonText(lang.get("projectManager.manage"));
    playProjectButton.setButtonText(lang.get("projectManager.play"));
    stopProjectButton.setButtonText(lang.get("projectManager.stop"));

    valueTreePropertyChanged(audioEngine.getProjectState(), {});
}

void ProjectManagerComponent::recordProject()
{
    auto& lang = LanguageManager::getInstance();
    auto* alert = new juce::AlertWindow(lang.get("projectManager.recordTitle"),
        lang.get("projectManager.recordMessage"),
        juce::AlertWindow::NoIcon);

    alert->addTextEditor("projectName", "New Project", lang.get("projectManager.projectNameLabel"));
    alert->addButton(lang.get("projectManager.recordButton"), 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton(lang.get("alerts.cancelButton"), 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result)
        {
            if (result == 1)
            {
                juce::String projectName = alert->getTextEditorContents("projectName").trim();
                if (projectName.isEmpty())
                {
                    recordProjectButton.setToggleState(false, juce::sendNotification);
                }
                else
                {
                    audioEngine.startProjectRecording(projectName);
                    updateTexts();
                }
            }
            else
            {
                recordProjectButton.setToggleState(false, juce::sendNotification);
            }
        }), true);
}

void ProjectManagerComponent::stopProjectRecording()
{
    audioEngine.stopProjectRecording();
    recordProjectButton.setToggleState(false, juce::dontSendNotification);
    updateTexts();
}

void ProjectManagerComponent::playProject()
{
    audioEngine.playLoadedProject();
}

void ProjectManagerComponent::stopProject()
{
    audioEngine.stopLoadedProject();
}

// <<< MODIFIED: This now correctly manages the window lifecycle >>>
void ProjectManagerComponent::manageProjects()
{
    if (projectListWindow == nullptr)
    {
        // Create the window and give it a callback to run on close.
        // The callback safely resets this component's unique_ptr.
        projectListWindow = std::make_unique<ProjectListWindow>(
            "Projects",
            audioEngine,
            [this](const juce::File& projectJsonFile) {
                audioEngine.loadProject(projectJsonFile);
            },
            [this] { // on window close
                projectListWindow.reset();
            }
        );
    }

    projectListWindow->toFront(true);
}