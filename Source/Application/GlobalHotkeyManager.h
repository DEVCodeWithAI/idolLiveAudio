#pragma once

#include <JuceHeader.h>
#include "../Data/SoundboardSlot.h"
#include <functional>

#if JUCE_WINDOWS
#define NOMINMAX
#include <Windows.h>

class GlobalHotkeyManager
{
public:
    GlobalHotkeyManager();
    ~GlobalHotkeyManager();

    void start();
    void stop();
    void updateHotkeys(const juce::Array<SoundboardSlot>& newSlots);

    // <<< ADDED: Function to enter capture mode >>>
    void setCaptureMode(std::function<void(const juce::KeyPress&)> onKeyCaptured);

    std::function<void(int)> onHotkeyTriggered;

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static GlobalHotkeyManager* instance;

    HHOOK hook = nullptr;
    juce::Array<SoundboardSlot> hotkeySlots;

    // <<< ADDED: Callback for capture mode >>>
    std::function<void(const juce::KeyPress&)> captureCallback;
};

#endif // JUCE_WINDOWS