#include "RecordingListWindow.h"
#include "../../Components/Helpers.h"

// ================== Item Component trong ListBox ==================
class RecordingItemComponent : public juce::Component
{
public:
    RecordingItemComponent(const juce::File& file, juce::Button::Listener* ownerListener) : recordingFile(file)
    {
        addAndMakeVisible(nameLabel);
        nameLabel.setText(recordingFile.getFileNameWithoutExtension(), juce::dontSendNotification);

        addAndMakeVisible(playButton);
        playButton.setButtonText(LanguageManager::getInstance().get("recordingListWindow.play"));
        playButton.addListener(ownerListener);
        playButton.setComponentID("play_" + recordingFile.getFileName());

        addAndMakeVisible(deleteButton);
        deleteButton.setButtonText(LanguageManager::getInstance().get("recordingListWindow.delete"));
        deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred.withAlpha(0.8f));
        deleteButton.addListener(ownerListener);
        deleteButton.setComponentID("delete_" + recordingFile.getFileName());
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
    juce::File recordingFile;
    juce::Label nameLabel;
    juce::TextButton playButton, deleteButton;
};


// ================== Nội dung chính của cửa sổ ==================
class RecordingListContentComponent : public juce::Component,
    public juce::ListBoxModel,
    public juce::Button::Listener
{
public:
    RecordingListContentComponent(const juce::String& subDir,
        std::function<void(const juce::File&)> onPlayCallback)
        : onPlay(onPlayCallback)
    {
        // Xây dựng đường dẫn đầy đủ tới thư mục chứa bản ghi
        auto rootDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile(ProjectInfo::companyName)
            .getChildFile(ProjectInfo::projectName)
            .getChildFile("Recordings");

        // Nếu có thư mục con được chỉ định, hãy đi vào trong
        if (subDir.isNotEmpty())
            recordingsDirectory = rootDir.getChildFile(subDir);
        else
            recordingsDirectory = rootDir;

        // Tạo thư mục nếu nó chưa tồn tại
        if (!recordingsDirectory.exists())
            recordingsDirectory.createDirectory();

        addAndMakeVisible(recordingListBox);
        recordingListBox.setModel(this);
        recordingListBox.setRowHeight(40);

        findRecordings();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {
        recordingListBox.setBounds(getLocalBounds().reduced(10));
    }

    void findRecordings()
    {
        recordedFiles.clear();
        recordingsDirectory.findChildFiles(recordedFiles, juce::File::findFiles, false, "*.wav");
        recordedFiles.sort();
        recordingListBox.updateContent();
        repaint();
    }

    int getNumRows() override { return recordedFiles.size(); }

    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}

    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override
    {
        juce::ignoreUnused(isRowSelected);
        if (!isPositiveAndBelow(rowNumber, recordedFiles.size()))
            return nullptr;

        // Tái sử dụng component cũ để tối ưu hiệu năng
        auto* item = static_cast<RecordingItemComponent*>(existingComponentToUpdate);
        if (item == nullptr)
            item = new RecordingItemComponent(recordedFiles.getReference(rowNumber), this);

        return item;
    }

    void buttonClicked(juce::Button* button) override
    {
        auto id = button->getComponentID();
        if (id.startsWith("play_"))
        {
            auto fileName = id.fromFirstOccurrenceOf("play_", false, false);
            auto file = recordingsDirectory.getChildFile(fileName);
            if (file.existsAsFile() && onPlay)
            {
                onPlay(file);
                if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
                    dw->closeButtonPressed();
            }
        }
        else if (id.startsWith("delete_"))
        {
            auto fileName = id.fromFirstOccurrenceOf("delete_", false, false);
            auto file = recordingsDirectory.getChildFile(fileName);

            if (file.existsAsFile())
            {
                juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                    "Confirm Delete",
                    "Are you sure you want to delete '" + file.getFileName() + "'?",
                    "Yes", "No", nullptr,
                    juce::ModalCallbackFunction::create([this, file](int result)
                        {
                            if (result == 1) // Yes
                            {
                                if (file.deleteFile())
                                    findRecordings();
                            }
                        }));
            }
        }
    }

private:
    juce::File recordingsDirectory;
    juce::ListBox recordingListBox;
    juce::Array<juce::File> recordedFiles;
    std::function<void(const juce::File&)> onPlay;
};

// ================== Triển khai Window ==================
RecordingListWindow::RecordingListWindow(const juce::String& recordingsSubDir,
    std::function<void(const juce::File&)> onPlayCallback)
    : DocumentWindow(LanguageManager::getInstance().get("recordingListWindow.title"),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(400, 300, 800, 1000);
    content = std::make_unique<RecordingListContentComponent>(recordingsSubDir, onPlayCallback);
    setContentOwned(content.get(), true);
    centreWithSize(500, 600);
}

RecordingListWindow::~RecordingListWindow() {
    content.reset();
}

void RecordingListWindow::closeButtonPressed()
{
    setVisible(false);
}

void RecordingListWindow::refreshList()
{
    if (content)
        dynamic_cast<RecordingListContentComponent*>(content.get())->findRecordings();
}