#include "RecorderComponent.h"
#include "../Windows/RecordingListWindow.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/AudioRecorder.h"

RecorderComponent::RecorderComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    audioEngine.getTransportSource().addChangeListener(this);

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(16.0f));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(loadedFileLabel);

    addAndMakeVisible(recordButton);
    recordButton.setClickingTogglesState(true);
    recordButton.onClick = [this] {
        if (recordButton.getToggleState())
            startRecording();
        else
            stopRecording();
        };

    addAndMakeVisible(playButton);
    playButton.onClick = [this] {
        if (state == PlayState::Paused)
            resumePlayback();
        else if (state == PlayState::Stopped)
            openRecordingList();
        };

    addAndMakeVisible(stopButton);
    stopButton.onClick = [this] {
        if (state == PlayState::Recording)
            stopRecording();
        else
            stopPlayback();
        };

    addAndMakeVisible(pauseButton);
    pauseButton.onClick = [this] { pausePlayback(); };

    addAndMakeVisible(recordedListButton);
    recordedListButton.onClick = [this] { openRecordingList(); };

    addAndMakeVisible(positionSlider);
    positionSlider.setRange(0.0, 1.0, 0.001);
    positionSlider.onDragEnd = [this] {
        if (state == PlayState::Playing || state == PlayState::Paused)
            audioEngine.getTransportSource().setPosition(positionSlider.getValue() * audioEngine.getTransportSource().getLengthInSeconds());
        };

    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.setRange(-60.0, 6.0, 0.1);
    volumeSlider.onValueChange = [this] {
        audioEngine.getTransportSource().setGain(juce::Decibels::decibelsToGain((float)volumeSlider.getValue()));
        };
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2d2d2d));


    addAndMakeVisible(currentTimeLabel);
    currentTimeLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(totalTimeLabel);
    totalTimeLabel.setJustificationType(juce::Justification::centredLeft);

    updateTexts();
    updateButtonStates();
    updateLoadedFileLabel();
    volumeSlider.setValue(juce::Decibels::gainToDecibels(audioEngine.getTransportSource().getGain()));
    startTimerHz(5);
}

RecorderComponent::~RecorderComponent()
{
    stopTimer();
    LanguageManager::getInstance().removeChangeListener(this);
    audioEngine.getTransportSource().removeChangeListener(this);
}

void RecorderComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff212121));
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 1.f);
}

void RecorderComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);

    // Khu vực mới cho tiêu đề và tên file
    auto titleArea = bounds.removeFromTop(25);
    titleLabel.setBounds(titleArea);
    bounds.removeFromTop(2);
    auto loadedFileArea = bounds.removeFromTop(10);
    loadedFileLabel.setBounds(loadedFileArea);
    bounds.removeFromTop(5);

    // Khu vực còn lại cho các control cũ
    auto buttonArea = bounds.removeFromTop(bounds.getHeight() / 2 - 10);
    bounds.removeFromTop(5);
    auto sliderArea = bounds;

    const int buttonGap = 5;
    const int listButtonWidth = 60;
    const int mainButtonsTotalWidth = buttonArea.getWidth() - listButtonWidth - (4 * buttonGap);
    const int mainButtonWidth = mainButtonsTotalWidth / 4;

    int currentX = buttonArea.getX();

    recordButton.setBounds(currentX, buttonArea.getY(), mainButtonWidth, buttonArea.getHeight());
    currentX += mainButtonWidth + buttonGap;

    playButton.setBounds(currentX, buttonArea.getY(), mainButtonWidth, buttonArea.getHeight());
    currentX += mainButtonWidth + buttonGap;

    pauseButton.setBounds(currentX, buttonArea.getY(), mainButtonWidth, buttonArea.getHeight());
    currentX += mainButtonWidth + buttonGap;

    stopButton.setBounds(currentX, buttonArea.getY(), mainButtonWidth, buttonArea.getHeight());

    recordedListButton.setBounds(buttonArea.getRight() - listButtonWidth, buttonArea.getY(), listButtonWidth, buttonArea.getHeight());

    auto positionSliderArea = sliderArea.removeFromTop(sliderArea.getHeight() / 2);
    sliderArea.removeFromTop(2);
    auto volumeSliderArea = sliderArea;

    currentTimeLabel.setBounds(positionSliderArea.removeFromLeft(50));
    totalTimeLabel.setBounds(positionSliderArea.removeFromRight(50));
    positionSlider.setBounds(positionSliderArea);

    volumeLabel.setBounds(volumeSliderArea.removeFromLeft(80));
    volumeSlider.setBounds(volumeSliderArea);
}

void RecorderComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
    else if (source == &audioEngine.getTransportSource())
    {
        if (audioEngine.getTransportSource().isPlaying())
        {
            state = PlayState::Playing;
        }
        else if (state != PlayState::Paused)
        {
            state = PlayState::Stopped;
            positionSlider.setValue(0.0, juce::dontSendNotification);
        }
        updateButtonStates();
    }
}

void RecorderComponent::timerCallback()
{
    if (state == PlayState::Playing || state == PlayState::Paused)
    {
        auto& transport = audioEngine.getTransportSource();
        double totalLength = transport.getLengthInSeconds();
        double currentPos = transport.getCurrentPosition();

        if (totalLength > 0 && !positionSlider.isMouseButtonDown())
            positionSlider.setValue(currentPos / totalLength, juce::dontSendNotification);

        if (!volumeSlider.isMouseButtonDown())
            volumeSlider.setValue(juce::Decibels::gainToDecibels(transport.getGain()), juce::dontSendNotification);

        currentTimeLabel.setText(formatTime(currentPos), juce::dontSendNotification);
        totalTimeLabel.setText(formatTime(totalLength), juce::dontSendNotification);
        totalTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    else if (state == PlayState::Recording)
    {
        auto currentRecordingTime = audioEngine.getAudioRecorder().getCurrentRecordingTime();
        currentTimeLabel.setText(formatTime(currentRecordingTime), juce::dontSendNotification);
        totalTimeLabel.setText("REC", juce::dontSendNotification);
        totalTimeLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        positionSlider.setValue(0.0, juce::dontSendNotification);
        currentTimeLabel.setText(formatTime(0), juce::dontSendNotification);
        totalTimeLabel.setText(formatTime(0), juce::dontSendNotification);
        totalTimeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
}

void RecorderComponent::updateButtonStates()
{
    recordButton.setToggleState(state == PlayState::Recording, juce::dontSendNotification);
    recordButton.setEnabled(state == PlayState::Stopped || state == PlayState::Recording);
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);

    playButton.setEnabled(state == PlayState::Stopped || state == PlayState::Paused);

    pauseButton.setEnabled(state == PlayState::Playing);
    stopButton.setEnabled(state != PlayState::Stopped);
    recordedListButton.setEnabled(state == PlayState::Stopped);
    volumeSlider.setEnabled(state != PlayState::Recording);
}

void RecorderComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    titleLabel.setText(lang.get("masterUtility.recAndPlayTitle"), juce::dontSendNotification);

    recordButton.setButtonText(lang.get("masterUtility.record"));
    playButton.setButtonText(lang.get("masterUtility.play"));
    pauseButton.setButtonText(lang.get("masterUtility.pause"));
    stopButton.setButtonText(lang.get("masterUtility.stop"));
    recordedListButton.setButtonText(lang.get("masterUtility.list"));
    volumeLabel.setText(lang.get("soundboard.volume"), juce::dontSendNotification);

    updateLoadedFileLabel();
}

juce::String RecorderComponent::formatTime(double seconds)
{
    if (seconds < 0) seconds = 0;
    auto totalSecs = static_cast<int>(seconds);
    auto mins = totalSecs / 60;
    auto secs = totalSecs % 60;
    return juce::String::formatted("%02d:%02d", mins, secs);
}

void RecorderComponent::openRecordingList()
{
    if (listWindow == nullptr)
    {
        listWindow = std::make_unique<RecordingListWindow>("", [this](const juce::File& fileToPlay) {
            startPlayback(fileToPlay);
            });
    }

    listWindow->refreshList();
    listWindow->setVisible(true);
    listWindow->toFront(true);
}

void RecorderComponent::startRecording()
{
    stopPlayback();
    state = PlayState::Recording;
    audioEngine.getAudioRecorder().startRecording();
    updateButtonStates();
    updateLoadedFileLabel();
}

void RecorderComponent::stopRecording()
{
    audioEngine.getAudioRecorder().stop();
    state = PlayState::Stopped;
    updateButtonStates();
    if (listWindow != nullptr)
        listWindow->refreshList();
    updateLoadedFileLabel();
}

void RecorderComponent::startPlayback(const juce::File& fileToPlay)
{
    audioEngine.startPlayback(fileToPlay);
    currentlyLoadedFile = fileToPlay;
    updateLoadedFileLabel();
}

void RecorderComponent::stopPlayback()
{
    audioEngine.stopPlayback();
    state = PlayState::Stopped; // Trực tiếp đặt trạng thái là Stopped
    currentlyLoadedFile = {}; // Xóa file
    updateLoadedFileLabel();
    updateButtonStates();       // Cập nhật lại giao diện ngay lập tức
}

void RecorderComponent::pausePlayback()
{
    if (state == PlayState::Playing)
    {
        audioEngine.pausePlayback();
        state = PlayState::Paused;
        updateButtonStates();
    }
}

void RecorderComponent::resumePlayback()
{
    if (state == PlayState::Paused)
    {
        audioEngine.resumePlayback();
    }
    else if (state == PlayState::Stopped)
    {
        if (currentlyLoadedFile.existsAsFile())
        {
            startPlayback(currentlyLoadedFile);
        }
        else
        {
            openRecordingList();
        }
    }
}

void RecorderComponent::updateLoadedFileLabel()
{
    if (state == PlayState::Recording)
    {
        // Sửa: Dùng juce::String() hoặc juce::String::empty để tạo chuỗi rỗng
        loadedFileLabel.setText("", juce::dontSendNotification);
    }
    else if (currentlyLoadedFile.existsAsFile())
    {
        loadedFileLabel.setText(currentlyLoadedFile.getFileName(), juce::dontSendNotification);
    }
    else
    {
        loadedFileLabel.setText(LanguageManager::getInstance().get("masterUtility.noFileLoaded"), juce::dontSendNotification);
    }
}