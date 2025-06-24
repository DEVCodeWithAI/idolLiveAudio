#pragma once

#include "JuceHeader.h"

// Lớp này định nghĩa giao diện và bảng màu tùy chỉnh cho ứng dụng,
// giúp tất cả các component có một vẻ ngoài đồng nhất.
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override;

    // THÊM: Khai báo override hàm vẽ Slider
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

private:
    // Tất cả các thiết lập màu sắc được thực hiện trong constructor.
    // Chúng ta có thể override các hàm vẽ (draw methods) ở đây sau nếu cần.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};