#include "StatusBarComponent.h"
#include "../../Components/Helpers.h"

StatusBarComponent::StatusBarComponent()
{
    LanguageManager::getInstance().addChangeListener(this);
    addAndMakeVisible(cpuLabel);
    addAndMakeVisible(latencyLabel);
    addAndMakeVisible(sampleRateLabel);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setJustificationType(juce::Justification::centred);

    // Placeholder texts
    cpuLabel.setText("CPU: --", juce::dontSendNotification);
    latencyLabel.setText("Latency: --", juce::dontSendNotification);
    sampleRateLabel.setText("Rate: --", juce::dontSendNotification);
}

StatusBarComponent::~StatusBarComponent() { LanguageManager::getInstance().removeChangeListener(this); }

void StatusBarComponent::setStatusMessage(const juce::String& message, bool isError)
{
    statusLabel.setText(message, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, isError ? juce::Colours::red : juce::Colours::white);
}

void StatusBarComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

void StatusBarComponent::resized() {
    auto bounds = getLocalBounds().reduced(10, 5);
    const int padding = 20;

    cpuLabel.setBounds(bounds.removeFromLeft(100));
    bounds.removeFromLeft(padding);
    latencyLabel.setBounds(bounds.removeFromLeft(150));
    bounds.removeFromLeft(padding);
    sampleRateLabel.setBounds(bounds.removeFromLeft(100));

    statusLabel.setBounds(bounds);
}

void StatusBarComponent::updateTexts() { }
void StatusBarComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    (void)source; // Báo cho compiler biết: có lý do để nó ở đây
}

void StatusBarComponent::updateStatus(double cpuUsage, double latencyMs, double sampleRate)
{
    // Định dạng chuỗi để hiển thị
    juce::String cpuText = "CPU: " + juce::String(cpuUsage, 1) + " %";
    juce::String latencyText = "Latency: " + juce::String(latencyMs, 2) + " ms";
    juce::String rateText = "Rate: " + juce::String(sampleRate / 1000.0, 1) + " kHz";

    // Cập nhật các label
    cpuLabel.setText(cpuText, juce::dontSendNotification);
    latencyLabel.setText(latencyText, juce::dontSendNotification);
    sampleRateLabel.setText(rateText, juce::dontSendNotification);
}