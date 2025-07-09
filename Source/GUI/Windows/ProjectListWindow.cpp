#include "ProjectListWindow.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/AudioEngine.h"

// ... (ProjectItemComponent class is unchanged) ...
class ProjectItemComponent : public juce::Component
{
public:
    ProjectItemComponent(juce::Button::Listener* owner)
    {
        auto& lang = LanguageManager::getInstance();
        addAndMakeVisible(nameLabel);
        addAndMakeVisible(playButton);
        playButton.setButtonText(lang.get("projectListWindow.play"));
        playButton.addListener(owner);
        addAndMakeVisible(deleteButton);
        deleteButton.setButtonText(lang.get("projectListWindow.delete"));
        deleteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkred.withAlpha(0.8f));
        deleteButton.addListener(owner);
    }

    void update(const juce::File& projectJson)
    {
        nameLabel.setText(projectJson.getFileNameWithoutExtension(), juce::dontSendNotification);
        playButton.setComponentID("play_" + projectJson.getFullPathName());
        deleteButton.setComponentID("delete_" + projectJson.getFullPathName());
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(5);
        deleteButton.setBounds(bounds.removeFromRight(80));
        bounds.removeFromRight(5);
        playButton.setBounds(bounds.removeFromRight(80));
        nameLabel.setBounds(bounds);
    }
private:
    juce::Label nameLabel;
    juce::TextButton playButton, deleteButton;
};


class ProjectListContentComponent : public juce::Component,
    public juce::ListBoxModel,
    public juce::Button::Listener
{
public:
    ProjectListContentComponent(const juce::String& subDir,
        AudioEngine& engine,
        std::function<void(const juce::File&)> onProjectChosenCallback)
        : audioEngine(engine),
        onProjectChosen(onProjectChosenCallback)
    {
        projectDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(ProjectInfo::companyName)
            .getChildFile(ProjectInfo::projectName)
            .getChildFile("Recordings")
            .getChildFile(subDir);

        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setRowHeight(40);
    }

    void resized() override
    {
        listBox.setBounds(getLocalBounds().reduced(10));
        if (!initialScanDone)
        {
            findProjects();
            initialScanDone = true;
        }
    }

    void findProjects()
    {
        projectJsonFiles.clear();
        projectDir.findChildFiles(projectJsonFiles, juce::File::findFiles, true, "*.json");
        listBox.updateContent();
    }

    int getNumRows() override { return projectJsonFiles.size(); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override
    {
        juce::ignoreUnused(isRowSelected);
        if (!isPositiveAndBelow(rowNumber, projectJsonFiles.size()))
            return nullptr;

        auto* item = static_cast<ProjectItemComponent*>(existingComponentToUpdate);
        if (item == nullptr)
            item = new ProjectItemComponent(this);

        item->update(projectJsonFiles.getReference(rowNumber));
        return item;
    }

    void buttonClicked(juce::Button* button) override
    {
        auto& lang = LanguageManager::getInstance();
        auto id = button->getComponentID();

        if (id.startsWith("play_"))
        {
            auto fullPath = id.fromFirstOccurrenceOf("play_", false, false);
            juce::File projectFile(fullPath);
            if (onProjectChosen && projectFile.existsAsFile())
                onProjectChosen(projectFile);

            if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
                dw->closeButtonPressed();
        }
        else if (id.startsWith("delete_"))
        {
            auto fullPath = id.fromFirstOccurrenceOf("delete_", false, false);
            juce::File projectFile(fullPath);
            auto projectFolder = projectFile.getParentDirectory();
            auto projectName = projectFolder.getFileName();

            if (projectFolder.isDirectory())
            {
                juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon, lang.get("projectListWindow.deleteConfirmTitle"),
                    lang.get("projectListWindow.deleteConfirmMessage").replace("{{projectName}}", projectName),
                    lang.get("presetbar.yes"), lang.get("presetbar.no"), nullptr,
                    juce::ModalCallbackFunction::create([this, projectFolder, projectName](int result) {
                        if (result == 1) // User clicked Yes
                        {
                            if (audioEngine.getProjectState().getProperty("name").toString() == projectName)
                            {
                                audioEngine.stopLoadedProject();
                            }

                            if (projectFolder.deleteRecursively())
                                findProjects();
                        }
                        }));
            }
        }
    }

private:
    AudioEngine& audioEngine;
    juce::File projectDir;
    juce::ListBox listBox;
    juce::Array<juce::File> projectJsonFiles;
    std::function<void(const juce::File&)> onProjectChosen;
    bool initialScanDone = false;
};


// <<< MODIFIED: Constructor now saves the on-close callback >>>
ProjectListWindow::ProjectListWindow(const juce::String& recordingsSubDir,
    AudioEngine& engine,
    std::function<void(const juce::File&)> onProjectChosen,
    std::function<void()> onWindowClosed)
    : DocumentWindow(LanguageManager::getInstance().get("projectListWindow.title"),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        allButtons),
    onWindowClosedCallback(onWindowClosed)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(400, 300, 800, 1000);
    content = std::make_unique<ProjectListContentComponent>(recordingsSubDir, engine, onProjectChosen);
    setContentOwned(content.get(), true);
    centreWithSize(500, 600);
    setVisible(true);
}

ProjectListWindow::~ProjectListWindow()
{
}

// <<< MODIFIED: Now calls the callback instead of deleting itself >>>
void ProjectListWindow::closeButtonPressed()
{
    if (onWindowClosedCallback)
        onWindowClosedCallback();
}

void ProjectListWindow::refreshList()
{
    if (auto* contentComp = dynamic_cast<ProjectListContentComponent*>(content.get()))
        contentComp->findProjects();
}