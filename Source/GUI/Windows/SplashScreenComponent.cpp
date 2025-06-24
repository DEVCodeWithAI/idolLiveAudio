#include "SplashScreenComponent.h"

//==============================================================================
SplashScreenComponent::SplashScreenComponent()
{
    DBG("[SPLASH] BinaryData::SplashScreen_pngSize = " << BinaryData::SplashScreen_pngSize);

    splashImage = juce::ImageFileFormat::loadFrom(BinaryData::SplashScreen_png, BinaryData::SplashScreen_pngSize);

    DBG("[SPLASH] splashImage.isValid(): " + juce::String((int)splashImage.isValid()));
}

SplashScreenComponent::~SplashScreenComponent()
{
}

void SplashScreenComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (splashImage.isValid())
    {
        g.setOpacity(1.0f);
        g.drawImage(splashImage, getLocalBounds().toFloat());
    }
    else
    {
        g.setColour(juce::Colours::yellow);
        g.setFont(24.0f);
        g.drawText("idolLiveAudio", getLocalBounds(), juce::Justification::centred, false);
    }

    // Text trạng thái - sát lề trái & lề dưới 2px
    g.setFont(14.0f);
    auto textArea = getLocalBounds();
    textArea.setLeft(textArea.getX() + 2);
    textArea.setBottom(textArea.getBottom() - 2);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawText(statusMessage, textArea.translated(1, 1), juce::Justification::bottomLeft, true);
    g.setColour(juce::Colours::white);
    g.drawText(statusMessage, textArea, juce::Justification::bottomLeft, true);
}

void SplashScreenComponent::setStatusMessage(const juce::String& message)
{
    // This can be called from a different thread, so use callAsync
    // to safely update the message and trigger a repaint on the message thread.
    juce::MessageManager::callAsync([w = juce::Component::SafePointer<SplashScreenComponent>(this), message]() {
        if (w != nullptr) {
            w->statusMessage = message;
            w->repaint();
        }
        });
}