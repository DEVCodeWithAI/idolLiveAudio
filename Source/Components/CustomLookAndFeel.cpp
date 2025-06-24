#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack); // Ẩn con trỏ mặc định
    setColour(juce::Slider::trackColourId, juce::Colour(0xff5a5a5a));
    setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
}

CustomLookAndFeel::~CustomLookAndFeel()
{
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        auto trackBounds = juce::Rectangle<float>(static_cast<float>(x), y + height * 0.5f - 3.0f, static_cast<float>(width), 6.0f);
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(trackBounds, 3.0f);

        auto valueBounds = trackBounds.withWidth(sliderPos - static_cast<float>(x));

        g.setColour(juce::Colour(0xff007acc));
        g.fillRoundedRectangle(valueBounds, 3.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}