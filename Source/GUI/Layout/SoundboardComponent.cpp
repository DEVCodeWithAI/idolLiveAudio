/*
  ==============================================================================

    SoundboardComponent.cpp
    (Fixed compiler errors)

  ==============================================================================
*/

#include "SoundboardComponent.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/SoundPlayer.h"

//==============================================================================
SoundboardComponent::SoundboardComponent(AudioEngine& engine)
    : audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    getSharedSoundboardProfileManager().addChangeListener(this);

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(IdolUIHelpers::createBoldFont(16.0f));
    titleLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.75);
    volumeSlider.onValueChange = [this]
        {
            audioEngine.getSoundPlayer().setGain(static_cast<float>(volumeSlider.getValue()));
        };

    addAndMakeVisible(enableButton);
    enableButton.setClickingTogglesState(true);
    enableButton.setToggleState(true, juce::dontSendNotification);
    enableButton.onClick = [this]
        {
            audioEngine.getSoundPlayer().setEnabled(enableButton.getToggleState());
            updateTexts();
        };

    addAndMakeVisible(stopAllButton);
    stopAllButton.onClick = [this]
        {
            audioEngine.getSoundPlayer().stopAll();
        };

    for (int i = 0; i < 9; ++i)
    {
        auto* button = gridButtons.add(new juce::TextButton());
        button->onClick = [this, i]
            {
                const auto& slots = getSharedSoundboardProfileManager().getCurrentSlots();
                if (juce::isPositiveAndBelow(i, slots.size()))
                {
                    const auto& slot = slots.getReference(i);
                    if (!slot.isEmpty())
                    {
                        audioEngine.getSoundPlayer().play(slot.audioFile, i);
                    }
                }
            };
            addAndMakeVisible(button);
    }

    updateTexts();
    updateButtonLabels();
}

SoundboardComponent::~SoundboardComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    getSharedSoundboardProfileManager().removeChangeListener(this);
}

void SoundboardComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff212121));
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 1.0f);
}

void SoundboardComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto topArea = bounds.removeFromTop(50);
    titleLabel.setBounds(topArea.removeFromTop(25));

    auto controlsArea = topArea;

    enableButton.setBounds(controlsArea.removeFromRight(100));
    controlsArea.removeFromRight(10);
    stopAllButton.setBounds(controlsArea.removeFromRight(100));
    controlsArea.removeFromRight(10);
    volumeLabel.setBounds(controlsArea.removeFromLeft(60));
    volumeSlider.setBounds(controlsArea);

    bounds.removeFromTop(10);

    auto gridArea = bounds;
    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.alignContent = juce::FlexBox::AlignContent::spaceBetween;

    const float itemWidth = gridArea.getWidth() / 3.1f;
    const float itemHeight = gridArea.getHeight() / 3.1f;

    for (auto* button : gridButtons)
    {
        fb.items.add(juce::FlexItem(*button).withWidth(itemWidth).withHeight(itemHeight));
    }

    fb.performLayout(gridArea);
}

void SoundboardComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
    else if (source == &getSharedSoundboardProfileManager())
    {
        updateButtonLabels();
    }
}

void SoundboardComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();

    titleLabel.setText(lang.get("soundboard.title"), juce::dontSendNotification);
    volumeLabel.setText(lang.get("soundboard.volume"), juce::dontSendNotification);

    // <<< FIX IS HERE: Removed second argument
    stopAllButton.setButtonText(lang.get("soundboard.stop"));

    if (enableButton.getToggleState())
        enableButton.setButtonText(lang.get("soundboard.enabled"));
    else
        enableButton.setButtonText(lang.get("soundboard.disabled"));

    updateButtonLabels();
}

void SoundboardComponent::updateButtonLabels()
{
    const auto& slots = getSharedSoundboardProfileManager().getCurrentSlots();
    for (int i = 0; i < gridButtons.size(); ++i)
    {
        if (auto* button = gridButtons[i])
        {
            if (juce::isPositiveAndBelow(i, slots.size()))
            {
                const auto& slot = slots.getReference(i);
                juce::String buttonText = slot.displayName;
                if (slot.hotkey.isValid())
                {
                    buttonText += "\n[" + slot.hotkey.getTextDescription() + "]";
                }
                button->setButtonText(buttonText);
                button->setEnabled(!slot.isEmpty());
            }
        }
    }
}