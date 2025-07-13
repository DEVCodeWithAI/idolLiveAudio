#include "BeatManagerComponent.h"
#include "../../Components/Helpers.h"

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

static juce::String removeAccents(const juce::String& text)
{
    juce::String result = text;

    // Chữ 'a'
    result = result.replaceCharacters("áàảãạăắằẳẵặâấầẩẫậ", "aaaaaaaaaaaaaaaaa");
    result = result.replaceCharacters("ÁÀẢÃẠĂẮẰẲẴẶÂẤẦẨẪẬ", "AAAAAAAAAAAAAAAAA");

    // Chữ 'e'
    result = result.replaceCharacters("éèẻẽẹêếềểễệ", "eeeeeeeeeee");
    result = result.replaceCharacters("ÉÈẺẼẸÊẾỀỂỄỆ", "EEEEEEEEEEE");

    // Chữ 'i'
    result = result.replaceCharacters("íìỉĩị", "iiiii");
    result = result.replaceCharacters("ÍÌỈĨỊ", "IIIII");

    // Chữ 'o'
    result = result.replaceCharacters("óòỏõọôốồổỗộơớờởỡợ", "ooooooooooooooooo");
    result = result.replaceCharacters("ÓÒỎÕỌÔỐỒỔỖỘƠỚỜỞỠỢ", "OOOOOOOOOOOOOOOOO");

    // Chữ 'u'
    result = result.replaceCharacters("úùủũụưứừửữự", "uuuuuuuuuuu");
    result = result.replaceCharacters("ÚÙỦŨỤƯỨỪỬỮỰ", "UUUUUUUUUUU");

    // Chữ 'y'
    result = result.replaceCharacters("ýỳỷỹỵ", "yyyyy");
    result = result.replaceCharacters("ÝỲỶỸỴ", "YYYYY");

    // Chữ 'đ'
    result = result.replaceCharacters("đ", "d");
    result = result.replaceCharacters("Đ", "D");

    return result.toLowerCase();
}


//==============================================================================
// Triển khai các hàm của BeatManagerComponent
//==============================================================================

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
    searchResultsBox.setModel(nullptr);
    LanguageManager::getInstance().removeChangeListener(this);
    if (searchEditor)
        searchEditor->removeListener(this);
}

void BeatManagerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff212121));
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
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.items.add(juce::FlexItem(openFolderButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(rescanButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(changeFolderButton).withFlex(1.0f));
    fb.performLayout(buttonRow);
    bounds.removeFromTop(gap);
    searchResultsBox.setBounds(bounds);
}

void BeatManagerComponent::initialisePaths()
{
    beatFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("Beats");
    if (!beatFolder.exists()) beatFolder.createDirectory();
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

// <<< HÀM NÀY ĐÃ ĐƯỢC SỬA LẠI ĐỂ AN TOÀN HƠN >>>
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
        fileChooser = std::make_unique<juce::FileChooser>(
            LanguageManager::getInstance().get("beatManager.selectFolderTitle"),
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "");

        auto* fc = fileChooser.get();

        // Sửa ở đây: Dùng juce::Component::SafePointer
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [safeThis = juce::Component::SafePointer(this), fc](const juce::FileChooser& chooser)
            {
                // Kiểm tra xem component còn tồn tại không
                if (safeThis == nullptr) return;

                if (fc == safeThis->fileChooser.get())
                {
                    auto chosenFolder = chooser.getResult();
                    if (chosenFolder.isDirectory())
                    {
                        safeThis->customScanFolder = chosenFolder;
                        safeThis->rescanBeats(safeThis->customScanFolder);
                    }
                }
            });
    }
}

void BeatManagerComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == searchEditor.get())
    {
        filterBeats();
    }
}

void BeatManagerComponent::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == searchEditor.get())
    {
        filterBeats(); // Gọi lọc trực tiếp
    }
}

void BeatManagerComponent::changeListenerCallback(juce::ChangeBroadcaster* source) { if (source == &LanguageManager::getInstance()) updateTexts(); }

void BeatManagerComponent::rescanBeats(juce::File folderToScan)
{
    allBeats.clear();
    if (!folderToScan.isDirectory()) folderToScan = beatFolder;

    juce::Array<juce::File> foundFiles;
    folderToScan.findChildFiles(foundFiles, juce::File::findFiles, false, "*.mp4");

    juce::Array<juce::var> jsonArray;
    for (const auto& file : foundFiles)
    {
        allBeats.add({ file.getFileNameWithoutExtension(), file });

        juce::DynamicObject::Ptr beatJson(new juce::DynamicObject());
        beatJson->setProperty("title", file.getFileNameWithoutExtension());
        beatJson->setProperty("path", file.getFullPathName());
        jsonArray.add(juce::var(beatJson.get()));
    }
    beatDatabaseFile.replaceWithText(juce::JSON::toString(jsonArray));
    filterBeats();
}

void BeatManagerComponent::loadBeats()
{
    allBeats.clear();
    if (beatDatabaseFile.existsAsFile())
    {
        auto parseResult = juce::JSON::parse(beatDatabaseFile);
        if (parseResult.isArray())
        {
            for (const auto& item : *parseResult.getArray())
            {
                if (item.isObject())
                {
                    allBeats.add({ item["title"].toString(), juce::File(item["path"].toString()) });
                }
            }
        }
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
            auto simplifiedTitle = removeAccents(beat.title);
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
        if (onBeatPlayed) onBeatPlayed();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, lang.get("beatManager.fileNotFoundTitle"), lang.get("beatManager.fileNotFoundMessage") + "\n" + fileToPlay.getFullPathName());
    }
}

void BeatManagerComponent::deleteBeat(int rowNumber)
{
    if (!isPositiveAndBelow(rowNumber, filteredBeats.size())) return;

    auto beatToDelete = filteredBeats[rowNumber];
    auto& lang = LanguageManager::getInstance();

    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
        lang.get("beatManager.deleteConfirmTitle"),
        lang.get("beatManager.deleteConfirmMessage") + "\n\"" + beatToDelete.title + "\"",
        lang.get("beatManager.deleteButton"),
        lang.get("beatManager.cancelButton"),
        nullptr,
        // Sửa ở đây: Dùng juce::Component::SafePointer
        juce::ModalCallbackFunction::create([safeThis = juce::Component::SafePointer(this), beatToDelete](int result)
            {
                // Kiểm tra xem component còn tồn tại không
                if (safeThis == nullptr) return;

                if (result == 1) // User clicked 'Delete'
                {
                    beatToDelete.file.deleteFile();

                    for (int i = 0; i < safeThis->allBeats.size(); ++i)
                    {
                        if (safeThis->allBeats.getReference(i).file == beatToDelete.file)
                        {
                            safeThis->allBeats.remove(i);
                            break;
                        }
                    }

                    juce::Array<juce::var> jsonArray;
                    for (const auto& beat : safeThis->allBeats)
                    {
                        juce::DynamicObject::Ptr beatJson(new juce::DynamicObject());
                        beatJson->setProperty("title", beat.title);
                        beatJson->setProperty("path", beat.file.getFullPathName());
                        jsonArray.add(juce::var(beatJson.get()));
                    }
                    safeThis->beatDatabaseFile.replaceWithText(juce::JSON::toString(jsonArray));

                    safeThis->filterBeats();
                }
            }));
}

int BeatManagerComponent::getNumRows() { return filteredBeats.size(); }
void BeatManagerComponent::paintListBoxItem(int, juce::Graphics&, int, int, bool) {}

juce::Component* BeatManagerComponent::refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate)
{
    juce::ignoreUnused(isRowSelected);
    if (!isPositiveAndBelow(rowNumber, filteredBeats.size())) return nullptr;

    const auto& beatData = filteredBeats.getReference(rowNumber);
    auto* itemComponent = static_cast<BeatItemComponent*>(existingComponentToUpdate);
    if (itemComponent == nullptr) itemComponent = new BeatItemComponent(*this);

    itemComponent->update(beatData.title, beatData.file);
    itemComponent->onDeleteClicked = [this, rowNumber] { deleteBeat(rowNumber); };

    return itemComponent;
}

void BeatManagerComponent::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    juce::ignoreUnused(row, event);
}