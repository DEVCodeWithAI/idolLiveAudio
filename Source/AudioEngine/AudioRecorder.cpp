#include "AudioRecorder.h"

AudioRecorder::AudioRecorder(juce::AudioFormatManager& formatManager, const juce::String& subDirectoryName)
    : formatManagerToUse(formatManager), subDirectory(subDirectoryName)
{
    backgroundThread.startThread();
}

AudioRecorder::~AudioRecorder()
{
    stop();
    backgroundThread.stopThread(5000);
}

juce::File AudioRecorder::getRecordingsDirectory()
{
    auto userDocs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto recordingDir = userDocs.getChildFile(ProjectInfo::companyName)
        .getChildFile(ProjectInfo::projectName)
        .getChildFile("Recordings");

    if (subDirectory.isNotEmpty())
        recordingDir = recordingDir.getChildFile(subDirectory);

    if (!recordingDir.exists())
        recordingDir.createDirectory();

    return recordingDir;
}

// Hàm này chỉ chuẩn bị file và trạng thái
void AudioRecorder::startRecording()
{
    stop();
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
    targetFileForNextWrite = getRecordingsDirectory().getChildFile("Rec_" + timestamp + ".wav");
    active = true;
    recordingStartTime = juce::Time::getMillisecondCounter();
}

// Hàm này cũng chỉ chuẩn bị file và trạng thái
void AudioRecorder::startRecording(const juce::File& targetFile)
{
    stop();
    targetFileForNextWrite = targetFile;
    active = true;
    recordingStartTime = juce::Time::getMillisecondCounter();
}

void AudioRecorder::stop()
{
    if (active.load())
    {
        active = false;
        recordingStartTime = 0;

        const juce::ScopedLock sl(writerCreationLock);
        writer.reset();
        targetFileForNextWrite = juce::File();
    }
}

// <<< LOGIC ĐÚNG NẰM Ở ĐÂY >>>
void AudioRecorder::processBlock(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (active.load())
    {
        const juce::ScopedLock sl(writerCreationLock);

        // Nếu writer chưa được tạo, hãy tạo nó ngay bây giờ với sampleRate chính xác
        if (writer == nullptr && sampleRate > 0 && targetFileForNextWrite.existsAsFile() == false)
        {
            writer.reset(formatManagerToUse.findFormatForFileExtension("wav")
                ->createWriterFor(targetFileForNextWrite.createOutputStream().release(),
                    sampleRate,
                    buffer.getNumChannels(),
                    16, {}, 0));
        }

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }
}

bool AudioRecorder::isRecording() const
{
    return active.load();
}

double AudioRecorder::getCurrentRecordingTime() const
{
    if (isRecording())
    {
        const auto elapsedTime = juce::Time::getMillisecondCounter() - recordingStartTime.load();
        return static_cast<double>(elapsedTime) / 1000.0;
    }
    return 0.0;
}