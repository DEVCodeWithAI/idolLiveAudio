#include "TrackPlayerComponent.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../Windows/RecordingListWindow.h"

// Thêm namespace cho các key của ValueTree để đồng bộ với AudioEngine
namespace ProjectStateIDs
{
    const juce::Identifier name("name");
    const juce::Identifier isPlaying("isPlaying");
}

TrackPlayerComponent::TrackPlayerComponent(PlayerType type, AudioEngine& engine)
    : playerType(type), audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    audioEngine.getTrackTransportSource(playerType).addChangeListener(this);
    // <<< ĐĂNG KÝ LẮNG NGHE TRẠNG THÁI PROJECT >>>
    audioEngine.getProjectState().addListener(this);

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(15.0f));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(recordButton);
    recordButton.setClickingTogglesState(true);
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    recordButton.onClick = [this] {
        auto& recorder = audioEngine.getTrackRecorder(playerType);
        if (recordButton.getToggleState())
        {
            recorder.startRecording();
            recordButton.setButtonText(juce::String::fromUTF8("● REC"));
        }
        else
        {
            recorder.stop();
            recordButton.setButtonText(LanguageManager::getInstance().get("trackPlayer.rec"));
        }
        };

    addAndMakeVisible(playButton);
    playButton.onClick = [this] {
        if (playState == State::Playing)
            pauseTrack();
        else
            resumeTrack();
        };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this] { stopTrack(); };

    addAndMakeVisible(listButton);
    listButton.onClick = [this] { openTrackRecordingList(); };

    addAndMakeVisible(loadButton);
    loadButton.onClick = [this] { loadFile(); };

    addAndMakeVisible(muteButton);
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this] {
        audioEngine.setTrackPlayerMute(playerType, muteButton.getToggleState());
        };

    addAndMakeVisible(positionSlider);
    positionSlider.setRange(0.0, 1.0, 0.001);
    positionSlider.onDragEnd = [this] {
        if (audioEngine.isProjectPlaybackActive())
        {
            audioEngine.seekProject(positionSlider.getValue());
        }
        else
        {
            auto& transport = audioEngine.getTrackTransportSource(playerType);
            if (transport.getLengthInSeconds() > 0)
                transport.setPosition(positionSlider.getValue() * transport.getLengthInSeconds());
        }
        };

    addAndMakeVisible(currentTimeLabel);
    addAndMakeVisible(totalTimeLabel);

    // Cập nhật giao diện lần đầu dựa trên trạng thái hiện tại
    valueTreePropertyChanged(audioEngine.getProjectState(), {});
    updateTexts();
    updatePlayButtonState();
    startTimerHz(5);
}

TrackPlayerComponent::~TrackPlayerComponent()
{
    listWindow.reset();
    stopTimer();
    LanguageManager::getInstance().removeChangeListener(this);
    audioEngine.getTrackTransportSource(playerType).removeChangeListener(this);
    // <<< HỦY ĐĂNG KÝ LẮNG NGHE >>>
    audioEngine.getProjectState().removeListener(this);
}

void TrackPlayerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff212121));
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 1.f);
}

void TrackPlayerComponent::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    titleLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);

    auto sliderArea = bounds.removeFromBottom(20);
    bounds.removeFromBottom(5);
    auto buttonArea = bounds;

    currentTimeLabel.setBounds(sliderArea.removeFromLeft(40));
    totalTimeLabel.setBounds(sliderArea.removeFromRight(40));
    positionSlider.setBounds(sliderArea);

    juce::FlexBox fb;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    fb.alignItems = juce::FlexBox::AlignItems::stretch;

    fb.items.add(juce::FlexItem(recordButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(playButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(stopButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(listButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(loadButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem(muteButton).withWidth(60.0f));

    fb.performLayout(buttonArea);
}

void TrackPlayerComponent::timerCallback()
{
    auto& transport = audioEngine.getTrackTransportSource(playerType);
    if (playState != State::Stopped)
    {
        double totalLength = transport.getLengthInSeconds();
        double currentPos = transport.getCurrentPosition();

        if (totalLength > 0 && !positionSlider.isMouseButtonDown())
        {
            positionSlider.setValue(currentPos / totalLength, juce::dontSendNotification);
        }
        currentTimeLabel.setText(formatTime(currentPos), juce::dontSendNotification);
        totalTimeLabel.setText(formatTime(totalLength), juce::dontSendNotification);
    }
    else
    {
        positionSlider.setValue(0.0, juce::dontSendNotification);
        currentTimeLabel.setText(formatTime(0), juce::dontSendNotification);
        totalTimeLabel.setText(formatTime(0), juce::dontSendNotification);
    }
}

void TrackPlayerComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
    else if (source == &audioEngine.getTrackTransportSource(playerType))
    {
        auto& transport = audioEngine.getTrackTransportSource(playerType);
        if (transport.isPlaying())
        {
            playState = State::Playing;
        }
        else
        {
            // <<< SỬA: Dùng logic đúng để kiểm tra xem track đã phát xong chưa >>>
            bool hasFinished = transport.getLengthInSeconds() > 0
                && transport.getCurrentPosition() >= transport.getLengthInSeconds();

            if (audioEngine.isProjectPlaybackActive() && hasFinished)
            {
                audioEngine.stopLoadedProject();
            }
            else if (playState == State::Playing)
            {
                playState = hasFinished ? State::Stopped : State::Paused;
                if (hasFinished)
                    stopTrack();
            }
        }
        updatePlayButtonState();
    }
}

void TrackPlayerComponent::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& /*property*/)
{
    if (tree.hasType("ProjectState"))
    {
        // Vô hiệu hóa các nút nếu project đang được tải hoặc đang phát
        bool projectModeActive = tree.getProperty(ProjectStateIDs::name).toString().isNotEmpty();
        setButtonsEnabled(!projectModeActive);
    }
}

void TrackPlayerComponent::setButtonsEnabled(bool shouldBeEnabled)
{
    recordButton.setEnabled(shouldBeEnabled);
    playButton.setEnabled(shouldBeEnabled);
    listButton.setEnabled(shouldBeEnabled);
    loadButton.setEnabled(shouldBeEnabled);

    // Nút Stop chỉ bật khi đang phát và không ở chế độ project
    stopButton.setEnabled(shouldBeEnabled && (playState != State::Stopped));
}

juce::String TrackPlayerComponent::formatTime(double seconds)
{
    if (seconds < 0) seconds = 0;
    auto totalSecs = static_cast<int>(seconds);
    auto mins = totalSecs / 60;
    auto secs = totalSecs % 60;
    return juce::String::formatted("%02d:%02d", mins, secs);
}

void TrackPlayerComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();

    titleLabel.setText(lang.get("trackPlayer.title"), juce::dontSendNotification);
    // Sửa: Chỉ đặt text cho nút REC nếu nó không đang ghi âm
    if (!recordButton.getToggleState())
        recordButton.setButtonText(lang.get("trackPlayer.rec"));

    updatePlayButtonState();
    stopButton.setButtonText(lang.get("trackPlayer.stop"));
    listButton.setButtonText(lang.get("trackPlayer.list"));
    loadButton.setButtonText(lang.get("trackPlayer.load"));
    muteButton.setButtonText(lang.get("tracks.mute"));
}

void TrackPlayerComponent::updatePlayButtonState()
{
    auto& lang = LanguageManager::getInstance();
    if (playState == State::Playing)
    {
        playButton.setButtonText(lang.get("trackPlayer.pause"));
    }
    else
    {
        playButton.setButtonText(lang.get("trackPlayer.play"));
    }
}

void TrackPlayerComponent::playTrack(const juce::File& fileToPlay)
{
    audioEngine.startTrackPlayback(playerType, fileToPlay);
}

void TrackPlayerComponent::stopTrack()
{
    audioEngine.stopTrackPlayback(playerType);
    playState = State::Stopped;
    updatePlayButtonState();
}

void TrackPlayerComponent::pauseTrack()
{
    audioEngine.pauseTrackPlayback(playerType);
}

void TrackPlayerComponent::resumeTrack()
{
    // Chỉ cho phép load file mới nếu không ở chế độ project
    if (audioEngine.getTrackTransportSource(playerType).getTotalLength() <= 0 && !audioEngine.isProjectPlaybackActive())
        loadFile();
    else
        audioEngine.resumeTrackPlayback(playerType);
}

void TrackPlayerComponent::loadFile()
{
    auto fc = std::make_shared<juce::FileChooser>("Load Audio File", juce::File{}, "*.mp3;*.wav;*.aif;*.aiff");
    fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, fc](const juce::FileChooser& chooser)
        {
            if (chooser.getResults().isEmpty()) return;
            playTrack(chooser.getResult());
        });
}

void TrackPlayerComponent::openTrackRecordingList()
{
    auto subDirName = (playerType == PlayerType::Vocal) ? "Vocal" : "Music";
    listWindow = std::make_unique<RecordingListWindow>(subDirName, [this](const juce::File& f) { playTrack(f); });
    listWindow->setVisible(true);
}