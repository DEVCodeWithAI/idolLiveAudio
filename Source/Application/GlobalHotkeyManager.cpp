#include "GlobalHotkeyManager.h"
#include "../Data/LanguageManager/LanguageManager.h"

#if JUCE_WINDOWS

// Define the static instance pointer
GlobalHotkeyManager* GlobalHotkeyManager::instance = nullptr;

GlobalHotkeyManager::GlobalHotkeyManager()
{
    instance = this;
}

GlobalHotkeyManager::~GlobalHotkeyManager()
{
    stop();
    instance = nullptr;
}

void GlobalHotkeyManager::start()
{
    auto& lang = LanguageManager::getInstance();
    juce::ignoreUnused(lang);
    if (hook == nullptr)
    {
        hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
        if (hook != nullptr)
        {
            DBG(lang.get("globalHotkeys.hookInstallOk"));
        }
        else
        {
            DBG(lang.get("globalHotkeys.hookInstallFail") + " Error: " + juce::String(GetLastError()));
        }
    }
}

void GlobalHotkeyManager::stop()
{
    if (hook != nullptr)
    {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
        DBG(LanguageManager::getInstance().get("globalHotkeys.hookUninstall"));
    }
}

void GlobalHotkeyManager::updateHotkeys(const juce::Array<SoundboardSlot>& newSlots)
{
    hotkeySlots = newSlots;
    DBG(LanguageManager::getInstance().get("globalHotkeys.managerUpdated").replace("{{count}}", juce::String(hotkeySlots.size())));
}

void GlobalHotkeyManager::setCaptureMode(std::function<void(const juce::KeyPress&)> onKeyCaptured)
{
    captureCallback = std::move(onKeyCaptured);
}

LRESULT CALLBACK GlobalHotkeyManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        if (instance != nullptr)
        {
            KBDLLHOOKSTRUCT* pkhs = (KBDLLHOOKSTRUCT*)lParam;

            // BƯỚC 1: Lọc chính xác các phím bổ trợ Ctrl, Alt, Shift
            const int relevantModifiersMask = juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::altModifier | juce::ModifierKeys::shiftModifier;
            const juce::ModifierKeys currentMods = juce::ModifierKeys::getCurrentModifiersRealtime().getRawFlags() & relevantModifiersMask;

            // BƯỚC 2: Tạo KeyPress với virtual key code gốc (đây là thay đổi quan trọng)
            // Chúng ta không còn đặt keyCode = 0 cho các phím bổ trợ nữa.
            const juce::KeyPress currentPress(pkhs->vkCode, currentMods, 0);

            // Ưu tiên chế độ bắt phím
            if (instance->captureCallback != nullptr)
            {
                juce::MessageManager::callAsync([press = currentPress]() {
                    if (GlobalHotkeyManager::instance != nullptr && GlobalHotkeyManager::instance->captureCallback != nullptr)
                    {
                        GlobalHotkeyManager::instance->captureCallback(press);
                        GlobalHotkeyManager::instance->captureCallback = nullptr;
                    }
                    });
                return 1; // Chặn không cho các ứng dụng khác nhận phím
            }

            // Nếu không bắt phím, kiểm tra trigger hotkey
            if (instance->onHotkeyTriggered != nullptr)
            {
                for (int i = 0; i < instance->hotkeySlots.size(); ++i)
                {
                    const auto& slot = instance->hotkeySlots.getReference(i);
                    if (slot.hotkey == currentPress && !slot.isEmpty())
                    {
                        juce::MessageManager::callAsync([i]() {
                            if (GlobalHotkeyManager::instance != nullptr && GlobalHotkeyManager::instance->onHotkeyTriggered)
                                GlobalHotkeyManager::instance->onHotkeyTriggered(i);
                            });

                        if (slot.consumeKey)
                        {
                            return 1; // Nếu người dùng muốn CHIẾM DỤNG, ta dừng ở đây
                        }

                    }
                }
            }
        }
    }
    return CallNextHookEx(instance ? instance->hook : nullptr, nCode, wParam, lParam);
}


#endif // JUCE_WINDOWS