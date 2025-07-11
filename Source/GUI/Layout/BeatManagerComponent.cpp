#include "BeatManagerComponent.h"
#include "../../Components/Helpers.h"

// ==============================================================================
// Lớp Editor tùy chỉnh để bắt sự kiện focus
class BeatManagerComponent::SearchEditor : public juce::TextEditor
{
public:
    SearchEditor() = default;
    std::function<void()> onFocusGained;
private:
    void focusGained(FocusChangeType cause) override
    {
        if (cause == FocusChangeType::focusChangedByMouseClick && onFocusGained)
            onFocusGained();
    }
};

// Lớp Item tùy chỉnh cho mỗi hàng trong ListBox
class BeatManagerComponent::BeatItemComponent : public juce::Component
{
public:
    std::function<void()> onDeleteClicked;

    BeatItemComponent(BeatManagerComponent& owner) : ownerComponent(owner)
    {
        addAndMakeVisible(titleLabel);
        addAndMakeVisible(playButton);
        addAndMakeVisible(deleteButton);
        deleteButton.setButtonText("X");
        deleteButton.setTooltip(LanguageManager::getInstance().get("beatManager.deleteTooltip"));
        deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred.withAlpha(0.9f));
        deleteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        deleteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        deleteButton.onClick = [this] { if (onDeleteClicked) onDeleteClicked(); };
    }

    void paint(juce::Graphics& g) override { g.fillAll(getLookAndFeel().findColour(juce::ListBox::backgroundColourId)); }

    void update(const juce::String& beatTitle, const juce::File& beatFile)
    {
        titleLabel.setText(beatTitle, juce::dontSendNotification);
        playButton.setButtonText(LanguageManager::getInstance().get("beatManager.playButton"));
        playButton.onClick = [this, beatFile] { ownerComponent.playBeat(beatFile); };
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(5);
        deleteButton.setBounds(bounds.removeFromRight(30));
        bounds.removeFromRight(5);
        playButton.setBounds(bounds.removeFromRight(80));
        titleLabel.setBounds(bounds);
    }

private:
    BeatManagerComponent& ownerComponent;
    juce::Label titleLabel;
    juce::TextButton playButton;
    juce::TextButton deleteButton;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeatItemComponent)
};

// Hàm trợ giúp để loại bỏ dấu tiếng Việt
static juce::String removeAccents(const juce::String& text)
{
    juce::String result = text.toLowerCase();
    result = result.replaceCharacters("áàảãạăắằẳẵặâấầẩẫậ", "aaaaaaaaaaaaaaaaa");
    result = result.replaceCharacters("éèẻẽẹêếềểễệ", "eeeeeeeeeee");
    result = result.replaceCharacters("íìỉĩị", "iiiii");
    result = result.replaceCharacters("óòỏõọôốồổỗộơớờởỡợ", "ooooooooooooooooo");
    result = result.replaceCharacters("úùủũụưứừửữự", "uuuuuuuuuuu");
    result = result.replaceCharacters("ýỳỷỹỵ", "yyyyy");
    result = result.replaceCharacter('đ', 'd');
    return result;
}

// ==============================================================================
// Triển khai các hàm của BeatManagerComponent

BeatManagerComponent::BeatManagerComponent()
{
    LanguageManager::getInstance().addChangeListener(this);
    initialisePaths();

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(16.0f));
    addAndMakeVisible(searchLabel);

    searchEditor = std::make_unique<SearchEditor>();
    addAndMakeVisible(*searchEditor);
    searchEditor->addListener(this);
    searchEditor->onFocusGained = [this] { if (onWantsToExpand) onWantsToExpand(); };

    addAndMakeVisible(searchButton);
    searchButton.addListener(this);
    addAndMakeVisible(openFolderButton);
    openFolderButton.addListener(this);
    addAndMakeVisible(rescanButton);
    rescanButton.addListener(this);
    addAndMakeVisible(changeFolderButton);
    changeFolderButton.addListener(this);

    addAndMakeVisible(searchResultsBox);
    searchResultsBox.setModel(this);
    searchResultsBox.setRowHeight(30);

    updateTexts();
    loadBeats();
    filterBeats();
}

BeatManagerComponent::~BeatManagerComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    searchResultsBox.setModel(nullptr);
}

void BeatManagerComponent::paint(juce::Graphics& g)
{
    // <<< THAY ĐỔI Ở ĐÂY >>>

    // 1. Tô màu nền tối, giống hệt như của Soundboard.
    g.fillAll(juce::Colour(0xff212121));

    // 2. Vẽ một đường viền bo tròn xung quanh.
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 1.0f);
}

void BeatManagerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const int rowHeight = 30;
    const int gap = 5;
    titleLabel.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(gap);
    auto searchRow = bounds.removeFromTop(rowHeight);
    searchLabel.setBounds(searchRow.removeFromLeft(60));
    searchButton.setBounds(searchRow.removeFromRight(80));
    searchRow.removeFromRight(gap);
    searchEditor->setBounds(searchRow);
    bounds.removeFromTop(gap);
    auto buttonRow = bounds.removeFromTop(rowHeight);

    // --- FIX IS HERE: Sử dụng FlexBox để chia đều các nút ---
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    // Thêm các nút vào FlexBox, mỗi nút sẽ co giãn với tỉ lệ bằng nhau (1.0f)
    fb.items.add(juce::FlexItem(openFolderButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(rescanButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(changeFolderButton).withFlex(1.0f));

    // Yêu cầu FlexBox thực hiện layout trên vùng chứa các nút
    fb.performLayout(buttonRow);
    // --- KẾT THÚC CHỈNH SỬA ---

    bounds.removeFromTop(gap);
    searchResultsBox.setBounds(bounds);
}

void BeatManagerComponent::initialisePaths()
{
    beatFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("Beats");

    if (!beatFolder.exists())
        beatFolder.createDirectory();

    beatDatabaseFile = beatFolder.getChildFile("beat_database.json");
}

void BeatManagerComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    titleLabel.setText(lang.get("beatManager.title"), juce::dontSendNotification);
    searchLabel.setText(lang.get("beatManager.searchLabel"), juce::dontSendNotification);
    searchButton.setButtonText(lang.get("beatManager.searchButton"));
    openFolderButton.setButtonText(lang.get("beatManager.openFolderButton"));
    rescanButton.setButtonText(lang.get("beatManager.rescanButton"));
    changeFolderButton.setButtonText(lang.get("beatManager.changeFolderButton"));
    searchResultsBox.updateContent();
}

void BeatManagerComponent::buttonClicked(juce::Button* button)
{
    if (button == &searchButton)
    {
        filterBeats();
    }
    else if (button == &openFolderButton)
    {
        if (customScanFolder.isDirectory())
            customScanFolder.revealToUser();
        else
            beatFolder.revealToUser();
    }
    else if (button == &rescanButton)
    {
        customScanFolder = {};
        rescanBeats();
    }
    else if (button == &changeFolderButton)
    {
        auto fc = std::make_shared<juce::FileChooser>(LanguageManager::getInstance().get("beatManager.selectFolderTitle"),
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "");

        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, fc](const juce::FileChooser& chooser)
            {
                auto chosenFolder = chooser.getResult();
                if (chosenFolder.isDirectory())
                {
                    customScanFolder = chosenFolder;
                    rescanBeats(customScanFolder);
                }
            });
    }
}

void BeatManagerComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == searchEditor.get()) filterBeats();
}

void BeatManagerComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == searchEditor.get()) filterBeats();
}

void BeatManagerComponent::rescanBeats(juce::File folderToScan)
{
    // <<< SỬA LỖI: Dọn dẹp các danh sách cũ TRƯỚC KHI tạo danh sách mới >>>
    filteredBeats.clear();
    allBeats.clear();
    searchResultsBox.updateContent(); // Cập nhật UI để xóa các hàng cũ

    if (!folderToScan.isDirectory())
        folderToScan = beatFolder;

    juce::Array<juce::File> foundFiles;
    folderToScan.findChildFiles(foundFiles, juce::File::findFiles, false, "*.mp4");

    for (const auto& file : foundFiles)
    {
        auto* beatInfo = new juce::DynamicObject();
        beatInfo->setProperty("title", file.getFileNameWithoutExtension());
        beatInfo->setProperty("path", file.getFullPathName());
        allBeats.add(juce::var(beatInfo));
    }

    beatDatabaseFile.replaceWithText(juce::JSON::toString(allBeats));

    // Mảng allBeats đã đúng, chỉ cần lọc và cập nhật UI
    filterBeats();
}

void BeatManagerComponent::loadBeats()
{
    if (beatDatabaseFile.existsAsFile())
    {
        auto parseResult = juce::JSON::parse(beatDatabaseFile);
        if (parseResult.isArray())
            allBeats = *parseResult.getArray();
    }
}

void BeatManagerComponent::filterBeats()
{
    filteredBeats.clear();
    auto searchTerm = searchEditor->getText().trim();
    if (searchTerm.isEmpty())
    {
        filteredBeats.addArray(allBeats);
    }
    else
    {
        auto simplifiedSearchTerm = removeAccents(searchTerm);
        for (const auto& beat : allBeats)
        {
            juce::String title = beat["title"].toString();
            auto simplifiedTitle = removeAccents(title);
            if (simplifiedTitle.contains(simplifiedSearchTerm))
            {
                filteredBeats.add(beat);
            }
        }
    }
    searchResultsBox.updateContent();
}

void BeatManagerComponent::playBeat(const juce::File& fileToPlay)
{
    auto& lang = LanguageManager::getInstance();
    if (fileToPlay.existsAsFile())
    {
        fileToPlay.startAsProcess();
        if (onBeatPlayed)
            onBeatPlayed();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            lang.get("beatManager.fileNotFoundTitle"),
            lang.get("beatManager.fileNotFoundMessage") + "\n" + fileToPlay.getFullPathName());
    }
}

void BeatManagerComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
}

void BeatManagerComponent::deleteBeat(int rowNumber)
{
    if (!isPositiveAndBelow(rowNumber, filteredBeats.size()))
        return;

    auto beatToDelete = filteredBeats[rowNumber];
    juce::String title = beatToDelete["title"];
    juce::File file(beatToDelete["path"].toString());

    auto& lang = LanguageManager::getInstance();

    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
        lang.get("beatManager.deleteConfirmTitle"),
        lang.get("beatManager.deleteConfirmMessage") + "\n\"" + title + "\"",
        lang.get("beatManager.deleteButton"),
        lang.get("beatManager.cancelButton"),
        nullptr,
        juce::ModalCallbackFunction::create([this, file, beatToDelete](int result)
            {
                if (result == 1)
                {
                    if (file.existsAsFile())
                        file.deleteFile();

                    for (int i = 0; i < allBeats.size(); ++i)
                    {
                        if (allBeats[i]["path"].toString() == beatToDelete["path"].toString())
                        {
                            allBeats.remove(i);
                            break;
                        }
                    }

                    beatDatabaseFile.replaceWithText(juce::JSON::toString(allBeats));

                    // Mảng allBeats đã đúng, chỉ cần lọc và cập nhật UI
                    filterBeats();
                }
            }));
}


int BeatManagerComponent::getNumRows() { return filteredBeats.size(); }
void BeatManagerComponent::paintListBoxItem(int, juce::Graphics&, int, int, bool) {}

juce::Component* BeatManagerComponent::refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    juce::ignoreUnused(isRowSelected);
    if (!isPositiveAndBelow(rowNumber, filteredBeats.size())) return nullptr;

    auto beatData = filteredBeats[rowNumber];
    juce::String title = beatData["title"];
    juce::File file(beatData["path"].toString());

    auto* itemComponent = static_cast<BeatItemComponent*>(existingComponentToUpdate);
    if (itemComponent == nullptr) itemComponent = new BeatItemComponent(*this);

    itemComponent->update(title, file);
    itemComponent->onDeleteClicked = [this, rowNumber] { deleteBeat(rowNumber); };

    return itemComponent;
}

void BeatManagerComponent::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    juce::ignoreUnused(row, event);
}