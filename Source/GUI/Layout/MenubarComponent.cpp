#include "MenubarComponent.h"
#include "../../AudioEngine/AudioEngine.h"

MenubarComponent::MenubarComponent(juce::AudioDeviceManager& dm, AudioEngine& engine)
    : deviceManager(dm), audioEngine(engine)
{
    LanguageManager::getInstance().addChangeListener(this);
    deviceManager.addChangeListener(this);

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

    // <<< MODIFIED SECTION START >>>
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
        // On non-Windows platforms, fall back to the original behavior
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
            if (selectedId == -1) // Nếu chọn "Không có đầu ra"
            {
                audioEngine.setSelectedOutputChannels(-1, -1);
            }
            // SỬA LỖI: Sử dụng selectedId - 1 làm chỉ số truy cập mảng
            else if (selectedId > 0 && (selectedId - 1) < availableOutputStereoStartIndices.size()) // Kiểm tra bounds chính xác hơn
            {
                const int selectedOutputLeftChannelIndex = availableOutputStereoStartIndices[selectedId - 1];
                audioEngine.setSelectedOutputChannels(selectedOutputLeftChannelIndex, selectedOutputLeftChannelIndex + 1);
            }
            else
            {
                // Trường hợp không mong muốn hoặc không có lựa chọn hợp lệ
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

void MenubarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);

    // Layout from right to left
    languageBox.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(10);
    vmPanelButton.setBounds(bounds.removeFromRight(150));
    bounds.removeFromRight(5);
    asioPanelButton.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(20);

    auto outputSelectorArea = bounds.removeFromRight(250);
    outputSelectorArea.removeFromLeft(10);
    outputChannelLabel.setBounds(outputSelectorArea.removeFromLeft(50));
    outputChannelSelector.setBounds(outputSelectorArea);

    audioSettingsButton.setBounds(bounds);
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
        populateOutputChannels(); // Khi thiết bị thay đổi, cập nhật cả kênh đầu ra
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
    // LƯU CƠ CHẾ ONCHANGE HIỆN TẠI VÀ TẠM TẮT NÓ
    auto originalOnChange = outputChannelSelector.onChange;
    outputChannelSelector.onChange = nullptr;

    outputChannelSelector.clear(juce::dontSendNotification);
    availableOutputStereoStartIndices.clear();

    outputChannelSelector.addItem(LanguageManager::getInstance().get("menubar.noOutput"), -1);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        auto deviceSetup = deviceManager.getAudioDeviceSetup(); // Lấy device setup
        juce::StringArray channelNames = device->getOutputChannelNames();

        // SỬA LỖI: Sử dụng một biến đếm riêng cho ComboBox ID để tạo ID tuần tự.
        int currentComboBoxId = 1; // Bắt đầu từ 1 cho các cặp kênh thực tế

        // Duyệt qua các kênh để tìm các cặp stereo đang hoạt động
        for (int i = 0; i < channelNames.size() - 1; i += 2)
        {
            // CHỈ THÊM NẾU CẢ HAI KÊNH TRONG CẶP ĐỀU ĐANG HOẠT ĐỘNG
            if (deviceSetup.outputChannels[i] && deviceSetup.outputChannels[i + 1])
            {
                juce::String pairName = channelNames[i] + " / " + channelNames[i + 1];
                outputChannelSelector.addItem(pairName, currentComboBoxId); // Sử dụng ID tuần tự
                availableOutputStereoStartIndices.add(i); // Lưu chỉ số kênh trái thực tế
                currentComboBoxId++; // Tăng ID cho mục tiếp theo
            }
            else
            {
                // No debug message
            }
        }
    }
    else
    {
        // No debug message
    }


    int selectedIdToSet = -1; // Mặc định là "No Output"
    int currentLeft = audioEngine.getSelectedOutputLeftChannel();
    int currentRight = audioEngine.getSelectedOutputRightChannel();
    bool currentSelectionFound = false;

    // Cố gắng tìm lại lựa chọn hiện tại nếu nó vẫn hợp lệ
    for (int i = 0; i < availableOutputStereoStartIndices.size(); ++i)
    {
        if (availableOutputStereoStartIndices[i] == currentLeft && (availableOutputStereoStartIndices[i] + 1) == currentRight)
        {
            selectedIdToSet = i + 1; // selectedIdToSet sẽ là ID tuần tự (1, 2, 3...)
            currentSelectionFound = true;
            break;
        }
    }

    // Nếu không tìm thấy lựa chọn hiện tại hoặc chưa có lựa chọn, chọn cặp đầu tiên nếu có
    if (!currentSelectionFound && !availableOutputStereoStartIndices.isEmpty())
    {
        selectedIdToSet = 1; // Chọn cặp kênh đầu tiên (ID 1)
    }
    else if (!currentSelectionFound && availableOutputStereoStartIndices.isEmpty())
    {
        // No debug message
    }

    // THIẾT LẬP LỰA CHỌN MÀ KHÔNG GỬI NOTIFICATION (vì ta sẽ tự gọi sau)
    outputChannelSelector.setSelectedId(selectedIdToSet, juce::dontSendNotification);

    // KHÔI PHỤC CƠ CHẾ ONCHANGE GỐC
    outputChannelSelector.onChange = originalOnChange;

    // GỌI ONCHANGE MỘT LẦN ĐỂ ĐẢM BẢO AUDIOENGINE CẬP NHẬT
    // Chỉ gọi nếu có một lựa chọn hợp lệ được thiết lập
    if (outputChannelSelector.onChange != nullptr && selectedIdToSet != -1) // Đảm bảo callback không null
    {
        outputChannelSelector.onChange(); // Gọi trực tiếp lambda
    }
    else if (outputChannelSelector.onChange == nullptr)
    {
        // No debug message
    }
}