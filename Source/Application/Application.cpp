#include "Application.h"
#include "../GUI/MainComponent/MainComponent.h"
#include "../GUI/Windows/PresetManagerWindow.h"
#include "../GUI/Windows/PluginManagerWindow.h"
#include "../GUI/Windows/SplashScreenComponent.h"
#include "../Data/AppState.h"
#include "../Data/LanguageManager/LanguageManager.h"
// Dòng #include "CrashHandler.h" đã được xóa


PluginManager& getSharedPluginManager()
{
    auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance());
    jassert(app != nullptr);
    auto* manager = app->getPluginManager();
    jassert(manager != nullptr);
    return *manager;
}
PresetManager& getSharedPresetManager()
{
    auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance());
    jassert(app != nullptr);
    auto* manager = app->getPresetManager();
    jassert(manager != nullptr);
    return *manager;
}

//==============================================================================

idolLiveAudioApplication::idolLiveAudioApplication() {}

idolLiveAudioApplication::MainWindow::MainWindow(juce::String name)
    : DocumentWindow(name,
        juce::Desktop::getInstance().getDefaultLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId),
        DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(), true);
#if JUCE_IOS || JUCE_ANDROID
    setFullScreen(true);
#else
    setResizable(true, true);
    setResizeLimits(1640, 1010, 10000, 10000);
    centreWithSize(1640, 1010);
#endif
    setVisible(false);
}

void idolLiveAudioApplication::MainWindow::closeButtonPressed()
{
    if (auto* mainComp = dynamic_cast<MainComponent*>(getContentComponent()))
    {
        mainComp->attemptToClose([](bool shouldQuit) {
            if (shouldQuit)
                JUCEApplication::getInstance()->quit();
            });
    }
    else
    {
        JUCEApplication::getInstance()->quit();
    }
}

// ... (Class SplashWindow không thay đổi) ...
class SplashWindow final : public juce::DocumentWindow
{
public:
    SplashWindow()
        : DocumentWindow({}, juce::Colours::transparentBlack, 0)
    {
        const float originalWidth = 950.0f;
        const float originalHeight = 530.0f;
        const float desiredWidth = 600.0f;
        const float aspectRatio = originalHeight / originalWidth;
        const int desiredHeight = static_cast<int>(desiredWidth * aspectRatio);

        setContentOwned(new SplashScreenComponent(), true);

        setUsingNativeTitleBar(false);
        setResizable(false, false);

        centreWithSize(static_cast<int>(desiredWidth), desiredHeight);

        setVisible(true);
    }

    SplashScreenComponent* getSplashContent()
    {
        return dynamic_cast<SplashScreenComponent*>(getContentComponent());
    }

    void setStatusMessage(const juce::String& msg)
    {
        if (auto* content = getSplashContent())
            content->setStatusMessage(msg);
    }

    void closeButtonPressed() override {}
};


//==============================================================================

void idolLiveAudioApplication::initialise(const juce::String& commandLine)
{
    // Lời gọi CrashHandler::install(); đã được xóa

    juce::ignoreUnused(commandLine);

    splashWindow = std::make_unique<SplashWindow>();
    const auto startTime = juce::Time::getMillisecondCounter();
    auto& lang = LanguageManager::getInstance();

    splashWindow->setStatusMessage(lang.get("splash.initManagers"));
    pluginManager = std::make_unique<PluginManager>();
    presetManager = std::make_unique<PresetManager>();

    splashWindow->setStatusMessage(lang.get("splash.createWindow"));
    mainWindow.reset(new MainWindow(getApplicationName()));

    if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
    {
        splashWindow->setStatusMessage(lang.get("splash.initAudio"));
        auto& deviceManager = mainComp->getAudioDeviceManager();
        auto sessionFile = AppState::getInstance().getSessionFile();
        std::unique_ptr<juce::XmlElement> deviceSettingsXml;

        if (sessionFile.existsAsFile())
        {
            if (auto parsedXml = juce::parseXML(sessionFile))
            {
                if (auto* customSetup = parsedXml->getChildByName("CUSTOM_AUDIO_SETUP"))
                {
                    deviceSettingsXml = std::make_unique<juce::XmlElement>("DEVICESETUP");
                    deviceSettingsXml->setAttribute("deviceType", customSetup->getStringAttribute("deviceType"));
                    deviceSettingsXml->setAttribute("audioInputDeviceName", customSetup->getStringAttribute("inputDevice"));
                    deviceSettingsXml->setAttribute("audioOutputDeviceName", customSetup->getStringAttribute("outputDevice"));
                }
            }
        }

        auto error = deviceManager.initialise(32, 32, deviceSettingsXml.get(), true);
        if (error.isNotEmpty()) { DBG("Audio device initialisation failed: " + error); }
    }

    splashWindow->setStatusMessage(lang.get("splash.finalizing"));
    const auto elapsedTime = juce::Time::getMillisecondCounter() - startTime;
    const int minDisplayTime = 4000;

    int timeToWait = std::max(0, minDisplayTime - (int)elapsedTime);

    auto* mainWin = mainWindow.get();
    juce::Timer::callAfterDelay(timeToWait, [this, mainWin]() mutable {
        splashWindow.reset();
        if (mainWin != nullptr)
            mainWin->setVisible(true);
        });
}

void idolLiveAudioApplication::shutdown()
{
    if (mainWindow != nullptr)
    {
        if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
            AppState::getInstance().saveState(*mainComp);
    }

    if (auto* mainComp = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
    {
        mainComp->getAudioDeviceManager().closeAudioDevice();
    }

    mainWindow = nullptr;
    presetManagerWindow = nullptr;
    pluginManagerWindow = nullptr;
    presetManager = nullptr;
    pluginManager = nullptr;
    splashWindow = nullptr;
}

void idolLiveAudioApplication::showPresetManagerWindow()
{
    if (presetManagerWindow == nullptr)
        presetManagerWindow = std::make_unique<PresetManagerWindow>();

    presetManagerWindow->setVisible(true);
    presetManagerWindow->toFront(true);
}

void idolLiveAudioApplication::showPluginManagerWindow()
{
    if (pluginManagerWindow == nullptr)
        pluginManagerWindow = std::make_unique<PluginManagerWindow>();

    pluginManagerWindow->setVisible(true);
    pluginManagerWindow->toFront(true);
}


START_JUCE_APPLICATION(idolLiveAudioApplication)