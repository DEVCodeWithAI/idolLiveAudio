#include "MenubarComponent.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Components/Helpers.h"
#include "../../Data/AppState.h"

// ... (Lớp LogoComponent không thay đổi) ...
class LogoComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        auto bounds = getLocalBounds().toFloat();
        auto mainTextBounds = bounds.removeFromTop(bounds.getHeight() * 0.65f);
        g.setFont(IdolUIHelpers::createBoldFont(28.0f));
        g.drawText("idolLiveAudio", mainTextBounds, juce::Justification::bottomLeft, false);
        g.setFont(IdolUIHelpers::createRegularFont(14.0f));
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Easy to live", bounds, juce::Justification::topLeft, false);
    }
};

// HÀM KHỞI TẠO (CONSTRUCTOR) ĐÃ SỬA
MenubarComponent::MenubarComponent(juce::AudioDeviceManager& dm, AudioEngine& engine)
    : deviceManager(dm), audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    deviceManager.addChangeListener(this);
    AppState::getInstance().addChangeListener(this);

    logo = std::make_unique<LogoComponent>();
    addAndMakeVisible(*logo);

    addAndMakeVisible(languageBox);
    addAndMakeVisible(asioPanelButton);
    addAndMakeVisible(vmPanelButton);
    addAndMakeVisible(audioSettingsButton);
    // <<< XÓA: outputChannelLabel và outputChannelSelector cũ >>>

    // <<< KHỞI TẠO COMPONENT MỚI >>>
    outputSelector = std::make_unique<ChannelSelectorComponent>(deviceManager, audioEngine, ChannelSelectorComponent::ChannelType::AudioOutputStereo, "menubar.outputChannels");
    addAndMakeVisible(*outputSelector);
    // Thiết lập callback
    outputSelector->onSelectionChange = [this](int newChannelIndex)
        {
            if (newChannelIndex == -1)
                audioEngine.setSelectedOutputChannels(-1, -1);
            else
                audioEngine.setSelectedOutputChannels(newChannelIndex, newChannelIndex + 1);
        };


    audioSettingsButton.onClick = [this] {
        auto* audioSelectorComponent = new juce::AudioDeviceSelectorComponent(
            deviceManager, 0, 256, 0, 256, false, false, true, false);
        audioSelectorComponent->setSize(600, 450);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(audioSelectorComponent);
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
        options.escapeKeyTriggersCloseButton = true;
        options.resizable = true;
        options.launchAsync();
        };

    vmPanelButton.onClick = [] {
#if JUCE_WINDOWS
        juce::File vmPath("C:\\Program Files (x86)\\VB\\Voicemeeter\\voicemeeterpro.exe");
        if (!vmPath.existsAsFile()) vmPath = juce::File("C:\\Program Files\\VB\\Voicemeeter\\voicemeeterpro.exe");
        if (vmPath.existsAsFile())
            vmPath.startAsProcess();
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Voicemeeter Not Found", "Could not find Voicemeeter Banana/Potato installation.");
#endif
        };

    asioPanelButton.onClick = [this] {
#if JUCE_WINDOWS
        juce::File asioPanel("C:\\Program Files (x86)\\ASIO4ALL v2\\a4apanel.exe");
        if (asioPanel.existsAsFile())
            asioPanel.startAsProcess();
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, LanguageManager::getInstance().get("alerts.asio4allNotFoundTitle"), LanguageManager::getInstance().get("alerts.asio4allNotFoundMessage"));
#else
        if (auto* device = deviceManager.getCurrentAudioDevice())
            device->showControlPanel();
        else
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "No device", "Please select an audio device first.");
#endif
        };

    languageBox.addItem("English", 1);
    languageBox.addItem(juce::String::fromUTF8("Tiếng Việt"), 2);
    languageBox.setSelectedId(1, juce::dontSendNotification);
    languageBox.onChange = [this] {
        LanguageManager::getInstance().loadLanguage(languageBox.getSelectedId() == 1 ? "en" : "vi");
        };

    updateTexts();
    updateButtonStates();
    // <<< XÓA: Lệnh gọi populateOutputChannels() cũ >>>
}

MenubarComponent::~MenubarComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    deviceManager.removeChangeListener(this);
    AppState::getInstance().removeChangeListener(this);
}

void MenubarComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

// HÀM RESIZED() ĐÃ SỬA
void MenubarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);

    logo->setBounds(bounds.removeFromLeft(300));
    bounds.removeFromLeft(350);
    audioSettingsButton.setBounds(bounds.removeFromLeft(150));

    languageBox.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(10);
    vmPanelButton.setBounds(bounds.removeFromRight(150));
    bounds.removeFromRight(5);
    asioPanelButton.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(20);

    // Phần còn lại cho Output Selector mới
    if (outputSelector)
        outputSelector->setBounds(bounds.reduced(0, 5));
}


void MenubarComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LanguageManager::getInstance())
    {
        updateTexts();
    }
    else if (source == &deviceManager)
    {
        updateButtonStates();
        // Không cần gọi populate nữa, vì component con sẽ tự xử lý
    }
    else if (source == &AppState::getInstance())
    {
        updateButtonStates();
    }
}

// <<< XÓA HOÀN TOÀN HÀM setSelectedOutputChannelPairByName VÀ populateOutputChannels >>>

void MenubarComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    asioPanelButton.setButtonText(lang.get("menubar.asioPanel"));
    vmPanelButton.setButtonText(lang.get("menubar.voicemeeterPanel"));
    audioSettingsButton.setButtonText(lang.get("menubar.audioSettings"));
    // <<< XÓA: Cập nhật text cho các label/combobox cũ >>>
    repaint();
}

void MenubarComponent::updateButtonStates()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    bool deviceIsRunning = (device != nullptr);
    const bool isLocked = AppState::getInstance().isSystemLocked(); // Lấy trạng thái khóa

    bool isASIO = deviceIsRunning && device->getTypeName().contains("ASIO");

    // <<< SỬA: Vô hiệu hóa các nút khi bị khóa >>>
    asioPanelButton.setEnabled(isASIO && !isLocked);
    audioSettingsButton.setEnabled(!isLocked);
    languageBox.setEnabled(!isLocked); // Thêm: Khóa luôn cả dropdown ngôn ngữ

    // Cập nhật trạng thái cho component mới, có kiểm tra trạng thái khóa
    if (outputSelector)
        outputSelector->setEnabled(deviceIsRunning && !isLocked);

    // Nút Voicemeeter chỉ hiện khi dùng Windows
#if JUCE_WINDOWS
    vmPanelButton.setVisible(true);
    vmPanelButton.setEnabled(!isLocked);
#else
    vmPanelButton.setVisible(false);
#endif
}