#include "FxSendsComponent.h"
#include "../../GUI/Layout/TrackComponent.h"
#include "../../Components/Helpers.h"

// ================== Component Tiêu Đề Tùy Chỉnh ==================
class FxSendsTitleComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        auto bounds = getLocalBounds().toFloat();

        // 1. Vẽ chữ "FX" to, đậm
        auto fxBounds = bounds.removeFromTop(bounds.getHeight() * 0.6f);
        g.setFont(IdolUIHelpers::createBoldFont(52.0f));
        g.drawText("FX", fxBounds, juce::Justification::centred, false);

        bounds.removeFromTop(2.0f);

        // 2. Vẽ chữ "SENDS" nhỏ ở dưới
        auto sendsBounds = bounds;
        g.setFont(IdolUIHelpers::createRegularFont(17.0f));
        g.drawText("SENDS", sendsBounds, juce::Justification::centred, false);
    }
};

// ================== Triển Khai FxSendsComponent ==================
FxSendsComponent::FxSendsComponent(TrackComponent& owner) : ownerTrack(owner)
{
    LanguageManager::getInstance().addChangeListener(this);

    // <<< SỬA: Khởi tạo component tùy chỉnh mới >>>
    fxSectionLabel = std::make_unique<FxSendsTitleComponent>();
    addAndMakeVisible(*fxSectionLabel);

    for (int i = 0; i < 4; ++i)
    {
        addAndMakeVisible(fxButtons[i]);
        fxButtons[i].setButtonText("FX " + juce::String(i + 1));
        const int index = i;
        fxButtons[i].onClick = [this, index] { ownerTrack.openFxWindow(index); };

        addAndMakeVisible(fxMuteButtons[i]);
        fxMuteButtons[i].setClickingTogglesState(true);
        fxMuteButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.withAlpha(0.9f));
        fxMuteButtons[i].onClick = [this, index] {
            ownerTrack.toggleFxMute(index, fxMuteButtons[index].getToggleState());
            updateMuteButtonState(index, fxMuteButtons[index].getToggleState());
            };
    }
    updateTexts();
}

FxSendsComponent::~FxSendsComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
}

void FxSendsComponent::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void FxSendsComponent::resized()
{
    auto bounds = getLocalBounds();

    // <<< SỬA: Căn chỉnh lại layout cho phù hợp >>>
    auto labelArea = bounds.removeFromLeft(90);
    // Căn giữa label theo chiều dọc
    fxSectionLabel->setBounds(labelArea.withSizeKeepingCentre(labelArea.getWidth(), 50));

    bounds.removeFromLeft(10);

    auto buttonsArea = bounds;
    const int numItems = 4;
    const int gap = 5;
    const int totalGapWidth = (numItems - 1) * gap;
    const int itemWidth = (buttonsArea.getWidth() - totalGapWidth) / numItems;

    for (int i = 0; i < 4; ++i)
    {
        auto itemBounds = buttonsArea.removeFromLeft(itemWidth);
        if (i < numItems - 1)
            buttonsArea.removeFromLeft(gap);

        auto fxButtonBounds = itemBounds.removeFromTop(static_cast<int>(itemBounds.getHeight() * 0.55f));
        itemBounds.removeFromTop(3);
        auto muteButtonBounds = itemBounds;

        fxButtons[i].setBounds(fxButtonBounds.reduced(2));
        fxMuteButtons[i].setBounds(muteButtonBounds.reduced(2));
    }
}


void FxSendsComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
}

void FxSendsComponent::updateMuteButtonState(int index, bool isMuted)
{
    if (isPositiveAndBelow(index, 4))
    {
        fxMuteButtons[index].setToggleState(isMuted, juce::dontSendNotification);
        fxMuteButtons[index].setButtonText(isMuted ?
            LanguageManager::getInstance().get("tracks.muted") :
            LanguageManager::getInstance().get("tracks.mute"));
    }
}

// Sửa: Hàm này giờ chỉ cần cập nhật các nút mute, không cần cập nhật label nữa
void FxSendsComponent::updateTexts()
{
    for (int i = 0; i < 4; ++i)
    {
        updateMuteButtonState(i, fxMuteButtons[i].getToggleState());
    }
}