#pragma once

#include <JuceHeader.h>
#include "../../Data/LanguageManager/LanguageManager.h"

class TrackComponent;

// Khai báo chuyển tiếp cho lớp tùy chỉnh mới
class FxSendsTitleComponent;

class FxSendsComponent : public juce::Component,
    private juce::ChangeListener
{
public:
    FxSendsComponent(TrackComponent& owner);
    ~FxSendsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateMuteButtonState(int index, bool isMuted);

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void updateTexts();

    TrackComponent& ownerTrack;

    // <<< SỬA: Thay thế Label bằng Component tùy chỉnh >>>
    std::unique_ptr<FxSendsTitleComponent> fxSectionLabel;

    std::array<juce::TextButton, 4> fxButtons;
    std::array<juce::TextButton, 4> fxMuteButtons;
};