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

    juce::FontOptions fontOptions(12.0f);

    addAndMakeVisible(attributionText);
    attributionText.setText("idolLiveAudio is a free and open-source software developed by ", juce::dontSendNotification);
    attributionText.setFont(juce::Font(fontOptions));
    attributionText.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    attributionText.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(linkText);
    linkText.setText("DEVCodeWithAI.", juce::dontSendNotification);
    juce::Font boldFont(fontOptions);
    boldFont.setBold(true);
    linkText.setFont(boldFont);

    linkText.setColour(juce::Label::textColourId, juce::Colour(0xff87CEEB));
    linkText.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(linkButton);
    linkButton.setButtonText("");
    linkButton.setTooltip("https://github.com/DEVCodeWithAI");
    linkButton.setURL(juce::URL("https://github.com/DEVCodeWithAI"));

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

    juce::TextLayout tl;
    juce::AttributedString linkAttrString;
    linkAttrString.setText(linkText.getText());
    linkAttrString.setFont(linkText.getFont());
    tl.createLayout(linkAttrString, 10000.0f);
    const int linkTextWidth = (int)tl.getWidth();

    juce::AttributedString staticAttrString;
    staticAttrString.setText(attributionText.getText());
    staticAttrString.setFont(attributionText.getFont());
    tl.createLayout(staticAttrString, 10000.0f);
    const int staticTextWidth = (int)tl.getWidth();

    const int totalAttributionWidth = staticTextWidth + linkTextWidth;

    auto rightBounds = bounds.removeFromRight(totalAttributionWidth);

    attributionText.setBounds(rightBounds.removeFromLeft(staticTextWidth));
    linkText.setBounds(rightBounds);

    linkButton.setBounds(linkText.getBounds());

    auto leftBounds = bounds;
    cpuLabel.setBounds(leftBounds.removeFromLeft(100));
    leftBounds.removeFromLeft(padding);
    latencyLabel.setBounds(leftBounds.removeFromLeft(150));
    leftBounds.removeFromLeft(padding);
    sampleRateLabel.setBounds(leftBounds.removeFromLeft(100));
    statusLabel.setBounds(leftBounds);
}

void StatusBarComponent::updateTexts() {}

void StatusBarComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    (void)source;
}

void StatusBarComponent::updateStatus(double cpuUsage, double latencyMs, double sampleRate)
{
    juce::String cpuText = "CPU: " + juce::String(cpuUsage, 1) + " %";
    juce::String latencyText = "Latency: " + juce::String(latencyMs, 2) + " ms";
    juce::String rateText = "Rate: " + juce::String(sampleRate / 1000.0, 1) + " kHz";

    cpuLabel.setText(cpuText, juce::dontSendNotification);
    latencyLabel.setText(latencyText, juce::dontSendNotification);
    sampleRateLabel.setText(rateText, juce::dontSendNotification);
}