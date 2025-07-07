#include "MenubarComponent.h"
#include "../../AudioEngine/AudioEngine.h"
#include "../../Components/Helpers.h" // Cần cho IdolUIHelpers

// ================== Component Logo Tùy Chỉnh ==================
class LogoComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        auto bounds = getLocalBounds().toFloat();

        // <<< SỬA: Giảm kích thước font cho phù hợp với chiều cao thanh menu >>>
        // Vẽ chữ "idolLiveAudio"
        auto mainTextBounds = bounds.removeFromTop(bounds.getHeight() * 0.65f);
        g.setFont(IdolUIHelpers::createBoldFont(28.0f)); // Giảm từ 42.0f
        g.drawText("idolLiveAudio", mainTextBounds, juce::Justification::bottomLeft, false);

        // Vẽ chữ "Easy to live"
        g.setFont(IdolUIHelpers::createRegularFont(14.0f)); // Giảm từ 18.0f
        g.setColour(juce::Colours::lightgrey);
        g.drawText("Easy to live", bounds, juce::Justification::topLeft, false);
    }
};


// ================== Triển Khai MenubarComponent ==================
MenubarComponent::MenubarComponent(juce::AudioDeviceManager& dm, AudioEngine& engine)
    : deviceManager(dm), audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    deviceManager.addChangeListener(this);

    // <<< THÊM: Khởi tạo Logo >>>
    logo = std::make_unique<LogoComponent>();
    addAndMakeVisible(*logo);

    addAndMakeVisible(languageBox);
    addAndMakeVisible(asioPanelButton);
    addAndMakeVisible(vmPanelButton);
    addAndMakeVisible(audioSettingsButton);
    addAndMakeVisible(outputChannelLabel);
    addAndMakeVisible(outputChannelSelector);

    audioSettingsButton.onClick = [this] {
        auto* audioSelectorComponent = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            0, 256, 0, 256,
            false, false, true, false);

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
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Voicemeeter Not Found",
                "Could not find Voicemeeter Banana/Potato installation.");
#endif
        };

    asioPanelButton.onClick = [this] {
#if JUCE_WINDOWS
        juce::File asioPanel("C:\\Program Files (x86)\\ASIO4ALL v2\\a4apanel.exe");

        if (asioPanel.existsAsFile())
        {
            asioPanel.startAsProcess();
        }
        else
        {
            auto& lang = LanguageManager::getInstance();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                lang.get("alerts.asio4allNotFoundTitle"),
                lang.get("alerts.asio4allNotFoundMessage"));
        }
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

    outputChannelSelector.onChange = [this]
        {
            const int selectedId = outputChannelSelector.getSelectedId();
            if (selectedId == -1)
            {
                audioEngine.setSelectedOutputChannels(-1, -1);
            }
            else if (selectedId > 0 && (selectedId - 1) < availableOutputStereoStartIndices.size())
            {
                const int selectedOutputLeftChannelIndex = availableOutputStereoStartIndices[selectedId - 1];
                audioEngine.setSelectedOutputChannels(selectedOutputLeftChannelIndex, selectedOutputLeftChannelIndex + 1);
            }
            else
            {
                audioEngine.setSelectedOutputChannels(-1, -1);
            }
        };

    updateTexts();
    updateButtonStates();
    populateOutputChannels();
}

MenubarComponent::~MenubarComponent()
{
    LanguageManager::getInstance().removeChangeListener(this);
    deviceManager.removeChangeListener(this);
}

void MenubarComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 1);
}

// <<< VIẾT LẠI HOÀN TOÀN HÀM RESIZED() >>>
void MenubarComponent::resized()
{
    // Thêm padding dọc để các component không bị dính vào cạnh trên và dưới
    auto bounds = getLocalBounds().reduced(10, 5);

    // --- Layout từ trái sang phải ---
    logo->setBounds(bounds.removeFromLeft(300));
    bounds.removeFromLeft(350);
    audioSettingsButton.setBounds(bounds.removeFromLeft(150));

    // --- Layout từ phải sang trái cho các nút còn lại ---
    languageBox.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(10);
    vmPanelButton.setBounds(bounds.removeFromRight(150));
    bounds.removeFromRight(5);
    asioPanelButton.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(20);

    // Phần còn lại cho Output Selector
    auto outputSelectorArea = bounds;
    // Căn giữa label và combobox theo chiều dọc
    outputChannelLabel.setBounds(outputSelectorArea.removeFromLeft(50).reduced(0, 5));
    outputSelectorArea.removeFromLeft(10);
    outputChannelSelector.setBounds(outputSelectorArea.reduced(0, 5));
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
        populateOutputChannels();
    }
}

void MenubarComponent::setSelectedOutputChannelPairByName(const juce::String& pairName)
{
    for (int i = 1; i <= outputChannelSelector.getNumItems(); ++i)
    {
        if (outputChannelSelector.getItemText(i - 1) == pairName)
        {
            outputChannelSelector.setSelectedId(i, juce::dontSendNotification);
            return;
        }
    }
}

void MenubarComponent::updateTexts()
{
    auto& lang = LanguageManager::getInstance();
    asioPanelButton.setButtonText(lang.get("menubar.asioPanel"));
    vmPanelButton.setButtonText(lang.get("menubar.voicemeeterPanel"));
    audioSettingsButton.setButtonText(lang.get("menubar.audioSettings"));
    outputChannelLabel.setText(lang.get("menubar.outputChannels"), juce::dontSendNotification);
    outputChannelSelector.setTextWhenNothingSelected(lang.get("menubar.noOutput"));
    repaint();
}

void MenubarComponent::updateButtonStates()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    bool deviceIsRunning = (device != nullptr);

    bool isASIO = deviceIsRunning && device->getTypeName().contains("ASIO");
    asioPanelButton.setEnabled(isASIO);

    audioSettingsButton.setEnabled(true);

    outputChannelSelector.setEnabled(deviceIsRunning);
}

void MenubarComponent::populateOutputChannels()
{
    auto originalOnChange = outputChannelSelector.onChange;
    outputChannelSelector.onChange = nullptr;

    outputChannelSelector.clear(juce::dontSendNotification);
    availableOutputStereoStartIndices.clear();

    outputChannelSelector.addItem(LanguageManager::getInstance().get("menubar.noOutput"), -1);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        auto deviceSetup = deviceManager.getAudioDeviceSetup();
        juce::StringArray channelNames = device->getOutputChannelNames();

        int currentComboBoxId = 1;

        for (int i = 0; i < channelNames.size() - 1; i += 2)
        {
            if (deviceSetup.outputChannels[i] && deviceSetup.outputChannels[i + 1])
            {
                juce::String pairName = channelNames[i] + " / " + channelNames[i + 1];
                outputChannelSelector.addItem(pairName, currentComboBoxId);
                availableOutputStereoStartIndices.add(i);
                currentComboBoxId++;
            }
        }
    }

    int selectedIdToSet = -1;
    int currentLeft = audioEngine.getSelectedOutputLeftChannel();
    int currentRight = audioEngine.getSelectedOutputRightChannel();
    bool currentSelectionFound = false;

    for (int i = 0; i < availableOutputStereoStartIndices.size(); ++i)
    {
        if (availableOutputStereoStartIndices[i] == currentLeft && (availableOutputStereoStartIndices[i] + 1) == currentRight)
        {
            selectedIdToSet = i + 1;
            currentSelectionFound = true;
            break;
        }
    }

    if (!currentSelectionFound && !availableOutputStereoStartIndices.isEmpty())
    {
        selectedIdToSet = 1;
    }

    outputChannelSelector.setSelectedId(selectedIdToSet, juce::dontSendNotification);

    outputChannelSelector.onChange = originalOnChange;

    if (outputChannelSelector.onChange != nullptr && selectedIdToSet != -1)
    {
        outputChannelSelector.onChange();
    }
}