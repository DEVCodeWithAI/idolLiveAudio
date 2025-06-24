/*
  ==============================================================================

    LevelMeter.cpp
    Created: 11 Jun 2025 8:39:17pm
    Author:  Kevin

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LevelMeter.h"

//==============================================================================
LevelMeter::LevelMeter()
{
    level = 0.0f;
    // Tăng tần số cập nhật để hoạt ảnh mượt hơn
    startTimerHz(30);
}

LevelMeter::~LevelMeter()
{
    stopTimer();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Vẽ nền
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(bounds, 4.0f);

    // === FIX START: Logic to draw HORIZONTAL meter ===

    // 2. Định nghĩa dải dB
    const float dbMin = -48.0f;
    const float dbMax = 6.0f;

    // 3. Chuyển đổi level đã làm mượt sang dB và vẽ thanh chính
    const float smoothedDb = juce::Decibels::gainToDecibels(smoothedLevel, dbMin);
    // Map the dB value to the component's WIDTH instead of height
    const float smoothedWidth = juce::jmap(smoothedDb, dbMin, dbMax, 0.0f, static_cast<float>(getWidth()));

    if (smoothedWidth > 0)
    {
        // Create a horizontal gradient fill
        juce::ColourGradient gradient(juce::Colour(0xff00b57e), // Green on the left
            bounds.getBottomLeft(),
            juce::Colour(0xfff84a4a), // Red on the right
            bounds.getBottomRight(),
            false);
        gradient.addColour(0.7, juce::Colour(0xfff2c200)); // Yellow in between

        g.setGradientFill(gradient);
        // Draw the rectangle from the left edge, with the calculated width
        g.fillRect(bounds.withWidth(smoothedWidth));
    }

    // 4. Chuyển đổi peak level sang dB và vẽ vạch chỉ báo đỉnh
    const float peakDb = juce::Decibels::gainToDecibels(peakLevel, dbMin);
    // Map the peak dB value to the component's WIDTH
    const float peakPositionX = juce::jmap(peakDb, dbMin, dbMax, 0.0f, static_cast<float>(getWidth()));

    if (peakPositionX > 0)
    {
        // Draw a thin vertical line for the peak indicator
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRect(peakPositionX, bounds.getY(), 2.0f, bounds.getHeight());
    }
    // === FIX END ===
}

void LevelMeter::resized()
{
}

//==============================================================================
void LevelMeter::timerCallback()
{
    // --- LOGIC CẬP NHẬT MỚI ---

    const float currentLevel = level.load();
    const float attackRate = 0.4f;  // Tốc độ thanh level đi lên
    const float releaseRate = 0.97f; // Tốc độ thanh level đi xuống

    // 1. Cập nhật smoothedLevel để tạo hoạt ảnh mượt
    if (currentLevel > smoothedLevel)
    {
        // Đi lên nhanh hơn
        smoothedLevel = currentLevel * attackRate + smoothedLevel * (1.0f - attackRate);
    }
    else
    {
        // Đi xuống chậm hơn
        smoothedLevel *= releaseRate;
    }

    // 2. Logic cho chỉ báo đỉnh (peak)
    const int peakHoldTime = 60; // Giữ trong 60 khung hình (khoảng 2 giây với 30Hz)

    if (currentLevel > peakLevel)
    {
        // Nếu level hiện tại cao hơn đỉnh, cập nhật đỉnh và reset bộ đếm
        peakLevel = currentLevel;
        peakHoldCountdown = peakHoldTime;
    }
    else
    {
        // Nếu không, bắt đầu đếm ngược
        if (peakHoldCountdown > 0)
        {
            --peakHoldCountdown;
        }
        else
        {
            // Khi hết thời gian giữ, cho đỉnh rơi xuống từ từ
            peakLevel *= 0.96f;
        }
    }

    repaint();
}

void LevelMeter::setLevel(float newLevel)
{
    level.store(newLevel);
}