#pragma once
#include <JuceHeader.h>

class AudioRecorder
{
public:
    AudioRecorder(juce::AudioFormatManager& formatManager, const juce::String& subDirectoryName);
    ~AudioRecorder();

    void startRecording();
    void startRecording(const juce::File& targetFile);

    void stop();
    void processBlock(const juce::AudioBuffer<float>& buffer, double sampleRate);
    bool isRecording() const;
    double getCurrentRecordingTime() const;

    juce::File getRecordingsDirectory();

private:
    juce::AudioFormatManager& formatManagerToUse;
    juce::TimeSliceThread backgroundThread{ "Audio Recorder Thread" };
    std::unique_ptr<juce::AudioFormatWriter> writer;
    std::atomic<bool> active{ false };
    std::atomic<int64_t> recordingStartTime{ 0 };

    juce::String subDirectory;

    // Các biến cần thiết cho logic "trì hoãn tạo writer"
    juce::File targetFileForNextWrite;
    juce::CriticalSection writerCreationLock;
};