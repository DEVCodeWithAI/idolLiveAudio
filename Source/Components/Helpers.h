#pragma once

#include <JuceHeader.h>

namespace IdolUIHelpers
{
    inline juce::Font createRegularFont(float height)
    {
        return juce::Font(juce::FontOptions().withHeight(height));
    }

    // Overload: Font bold với tên
    inline juce::Font createBoldFont(const juce::String& name, float height)
    {
#pragma warning(push)
#pragma warning(disable : 4996)
        juce::Font font(name, height, juce::Font::bold);
#pragma warning(pop)
        return font;
    }

    // Overload: Font bold mặc định
    inline juce::Font createBoldFont(float height)
    {
        return createBoldFont("Default", height);
    }
}
