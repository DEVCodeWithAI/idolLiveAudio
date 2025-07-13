/*
  ==============================================================================

    SoundboardComponent.cpp
    (FIXED: Added missing include for Windows VK_ codes)

  ==============================================================================
*/

#include "SoundboardComponent.h"
#include "../../Components/Helpers.h"
#include "../../AudioEngine/SoundPlayer.h"
#include "../../Application/Application.h" 

//==============================================================================
// CÁC HÀM TRỢ GIÚP ĐỂ HIỂN THỊ ĐÚNG TÊN HOTKEY
//==============================================================================

static juce::String getReadableKeyNameFromVK(int vkCode)
{
    switch (vkCode)
    {
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return "Ctrl";
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:       return "Shift";
    case VK_MENU: case VK_LMENU: case VK_RMENU:          return "Alt";
    case VK_LWIN: case VK_RWIN:                          return "Win";

    case VK_RETURN: return "Enter";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE:  return "Space";
    case VK_BACK:   return "Backspace";
    case VK_TAB:    return "Tab";
    case VK_DELETE: return "Delete";

    case VK_LEFT:   return "Left";
    case VK_RIGHT:  return "Right";
    case VK_UP:     return "Up";
    case VK_DOWN:   return "Down";

    case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
    case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
    case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
    case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";

    default:
        if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9'))
            return juce::String::charToString((juce_wchar)vkCode);
    }

    return "[?]";
}

static juce::String formatVKHotkeyText(const juce::KeyPress& key)
{
    if (!key.isValid())
        return {};

    juce::StringArray parts;
    const auto mods = key.getModifiers();

    if (mods.isCtrlDown())    parts.add("Ctrl");
    if (mods.isShiftDown())   parts.add("Shift");
    if (mods.isAltDown())     parts.add("Alt");
    if (mods.isCommandDown()) parts.add("Win");

    const int vkCode = key.getKeyCode();
    juce::String mainKey = getReadableKeyNameFromVK(vkCode);

    if (!parts.contains(mainKey))
        parts.add(mainKey);

    return parts.joinIntoString(" + ");
}


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
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.75);

    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black.withAlpha(0.5f));
    volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2d2d2d));

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

                juce::String hotkeyString = formatVKHotkeyText(slot.hotkey);
                juce::String buttonText = slot.displayName;

                if (hotkeyString.isNotEmpty())
                {
                    buttonText += "\n[" + hotkeyString + "]";
                }

                button->setButtonText(buttonText);
                button->setEnabled(!slot.isEmpty());
            }
        }
    }
}