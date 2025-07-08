#pragma once
#include "JuceHeader.h"
#include "../../Data/LanguageManager/LanguageManager.h"

class StatusBarComponent : public juce::Component, public juce::ChangeListener {
public:
    StatusBarComponent();
    ~StatusBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setStatusMessage(const juce::String& message, bool isError);
    void updateStatus(double cpuUsage, double latencyMs, double sampleRate);

private:
    void updateTexts();

    juce::Label cpuLabel, latencyLabel, sampleRateLabel;
    juce::Label statusLabel;

    // <<< SỬA: Dùng 2 Label riêng biệt >>>
    juce::Label attributionText; // Cho phần text thường
    juce::Label linkText;        // Cho phần text in đậm
    juce::HyperlinkButton linkButton;  // Vẫn là button vô hình
};