// Source/GUI/Layout/PluginManagementComponent.cpp

#include "PluginManagementComponent.h"
#include "../../Application/Application.h"

// Helper class for the scanning progress window (unchanged)
class ScanningWindow
{
public:
    ScanningWindow(const juce::String& title, const juce::String& message)
    {
        alertWindow = std::make_unique<juce::AlertWindow>(title, message, juce::AlertWindow::NoIcon);
        alertWindow->enterModalState(false);
    }

    ~ScanningWindow()
    {
        juce::MessageManager::callAsync([w = std::move(alertWindow)]() mutable {
            if (w != nullptr)
            {
                w->exitModalState(0);
            }
            });
    }
private:
    std::unique_ptr<juce::AlertWindow> alertWindow;
};


PluginManagementComponent::PluginManagementComponent() {
    LanguageManager::getInstance().addChangeListener(this);

    addAndMakeVisible(scanPluginsButton);
    scanPluginsButton.onClick = [this] {
        startScan();
        };

    addAndMakeVisible(managePluginsButton);
    managePluginsButton.onClick = [] {
        if (auto* app = dynamic_cast<idolLiveAudioApplication*>(juce::JUCEApplication::getInstance()))
            app->showPluginManagerWindow();
        };

    addAndMakeVisible(addPluginButton);
    addPluginButton.onClick = [] {
        getSharedPluginManager().addPluginFromUserChoice();
        };

    updateTexts();
}

PluginManagementComponent::~PluginManagementComponent() {
    stopTimer(); // Ensure timer is stopped on destruction
    LanguageManager::getInstance().removeChangeListener(this);
}


// <<< NEW: Method to setup and start the non-blocking scan >>>
void PluginManagementComponent::startScan()
{
    scanPluginsButton.setEnabled(false);
    auto& lang = LanguageManager::getInstance();
    scanningWindow = std::make_unique<ScanningWindow>(lang.get("scanProcess.title"), lang.get("scanProcess.message"));

    // Reset state
    filesToScan.clear();
    nextFileToScan = 0;
    tempList.clear();

    // 1. Find all VST3 files, just like in the previous robust fix
    juce::VST3PluginFormat vst3Format;
    auto searchPaths = vst3Format.getDefaultLocationsToSearch();
    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        searchPaths[i].findChildFiles(filesToScan, juce::File::findFiles, true, "*.vst3");
    }

    // 2. Start the timer to process files one by one
    startTimerHz(10); // Process up to 10 files per second
}


// <<< NEW: This is the heart of the non-blocking scan >>>
void PluginManagementComponent::timerCallback()
{
    // This runs on the main message thread, but in small chunks

    if (nextFileToScan < filesToScan.size())
    {
        const auto& pluginFile = filesToScan.getReference(nextFileToScan);

        // Skip waveshells, they will be handled by the dedicated manager
        if (!pluginFile.getFileName().containsIgnoreCase("WaveShell"))
        {
            // Scanning is safe here because we are on the main thread
            try
            {
                juce::VST3PluginFormat vst3Format;
                juce::OwnedArray<juce::PluginDescription> descs;
                vst3Format.findAllTypesForFile(descs, pluginFile.getFullPathName());

                for (auto* desc : descs)
                    if (desc != nullptr)
                        tempList.addType(*desc);
            }
            catch (...)
            {
                DBG("An exception was caught while scanning a non-Waves plugin: " + pluginFile.getFileName() + ". Skipping this file.");
            }
        }

        ++nextFileToScan; // Move to the next file
    }
    else
    {
        // --- All non-Waves files have been scanned ---
        stopTimer();

        // 3. Now, handle Waves plugins using the dedicated, safe manager.
        // This is still a blocking call, but it's much faster than a full scan.
        WavesShellManager::getInstance().scanAndParseShells();
        for (const auto& wavesDesc : WavesShellManager::getInstance().getScannedPlugins())
        {
            tempList.addType(wavesDesc);
        }

        // 4. Update the actual plugin list in the manager
        getSharedPluginManager().replaceAllPlugins(tempList);

        // 5. Clean up UI
        scanningWindow.reset();
        scanPluginsButton.setEnabled(true);

        auto& lang = LanguageManager::getInstance();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            lang.get("scanProcess.completeTitle"),
            lang.get("scanProcess.completeMessage"));
    }
}


void PluginManagementComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void PluginManagementComponent::resized() {
    auto bounds = getLocalBounds().reduced(15, 10);

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    fb.items.add(juce::FlexItem(scanPluginsButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10)); // Spacer
    fb.items.add(juce::FlexItem(managePluginsButton).withFlex(1.0f));
    fb.items.add(juce::FlexItem().withWidth(10)); // Spacer
    fb.items.add(juce::FlexItem(addPluginButton).withFlex(1.0f));

    fb.performLayout(bounds);
}

void PluginManagementComponent::updateTexts() {
    auto& lang = LanguageManager::getInstance();
    scanPluginsButton.setButtonText(lang.get("pluginManagement.scan"));
    managePluginsButton.setButtonText(lang.get("pluginManagement.manage"));
    addPluginButton.setButtonText(lang.get("pluginManagement.add"));
}

void PluginManagementComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &LanguageManager::getInstance()) {
        updateTexts();
    }
}