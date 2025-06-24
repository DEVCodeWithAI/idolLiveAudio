/*
  ==============================================================================

    PluginEditorWindow.h
    (Final attempt: Forcing plugin preparation before creating UI)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "AudioEngine/ProcessorBase.h"
#include "Data/LanguageManager/LanguageManager.h"

class PluginEditorWindow : public juce::DocumentWindow,
    public juce::AudioProcessorListener,
    public juce::ChangeListener
{
public:
    PluginEditorWindow(ProcessorBase& ownerProcessor, juce::AudioPluginInstance& plugin)
        : DocumentWindow(plugin.getName(),
            juce::Desktop::getInstance().getDefaultLookAndFeel()
            .findColour(juce::ResizableWindow::backgroundColourId),
            allButtons),
        processor(ownerProcessor),
        ownerPlugin(plugin)
    {
        setSize(500, 400);
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setVisible(true);
        centreWithSize(getWidth(), getHeight());

        ownerPlugin.addListener(this);
        processor.addChangeListener(this);

        juce::Component::SafePointer<PluginEditorWindow> safeThis(this);
        juce::AudioPluginInstance* pluginPtr = &plugin;

        juce::Timer::callAfterDelay(100, [safeThis, pluginPtr]
            {
                const juce::MessageManagerLock mmLock;
                if (!mmLock.lockWasGained() || safeThis == nullptr || pluginPtr == nullptr)
                    return;

                try
                {
                    // <<< APPLYING YOUR TIER 1 SOLUTION: Force preparation state >>>
                    // Ensure the plugin is not suspended and has been prepared for play.
                    // Some plugins require this before they will create an editor.
                    if (pluginPtr->isSuspended())
                        pluginPtr->suspendProcessing(false);

                    if (pluginPtr->getSampleRate() < 1)
                    {
                        auto spec = safeThis->getOwnerProcessor().getProcessSpec();
                        if (spec.sampleRate > 0)
                        {
                            DBG("Plugin was not prepared. Preparing it now before creating editor...");
                            pluginPtr->prepareToPlay(spec.sampleRate, spec.maximumBlockSize);
                        }
                    }
                    // --- END OF TIER 1 FIX ---


                    if (auto* editor = pluginPtr->createEditorIfNeeded())
                    {
                        safeThis->setContentOwned(editor, true);

                        if (auto* constrainer = editor->getConstrainer())
                        {
                            int minW = constrainer->getMinimumWidth();
                            int minH = constrainer->getMinimumHeight();
                            int maxW = constrainer->getMaximumWidth();
                            int maxH = constrainer->getMaximumHeight();

                            if (minW > 0 && minH > 0 && maxW >= minW && maxH >= minH)
                                safeThis->setResizeLimits(minW, minH, maxW, maxH);
                        }

                        int w = editor->getWidth();
                        int h = editor->getHeight();
                        if (w > 0 && h > 0)
                        {
                            safeThis->setSize(w, h);
                            safeThis->centreWithSize(w, h);
                        }
                    }
                    else
                    {
                        throw std::runtime_error("createEditorIfNeeded() returned nullptr after preparation attempt.");
                    }
                }
                catch (const std::exception& e)
                {
                    auto& lang = LanguageManager::getInstance();
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, lang.get("alerts.pluginErrorTitle"), "Failed to create UI: " + juce::String(e.what()));
                    if (safeThis != nullptr) safeThis->closeButtonPressed();
                }
                catch (...)
                {
                    auto& lang = LanguageManager::getInstance();
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, lang.get("alerts.pluginErrorTitle"), lang.get("alerts.pluginCrashUnknown"));
                    if (safeThis != nullptr) safeThis->closeButtonPressed();
                }
            });
    }

    // This new getter is needed for the fix above
    ProcessorBase& getOwnerProcessor() { return processor; }

    ~PluginEditorWindow() override
    {
        ownerPlugin.removeListener(this);
        processor.removeChangeListener(this);
        clearContentComponent();
    }

    juce::AudioPluginInstance& getOwnerPlugin() const { return ownerPlugin; }

    void closeButtonPressed() override
    {
        juce::MessageManager::callAsync([this] { delete this; });
    }

    void audioProcessorParameterChanged(juce::AudioProcessor*, int, float) override {}

    void audioProcessorChanged(juce::AudioProcessor*, const juce::AudioProcessor::ChangeDetails& details) override
    {
        if (details.programChanged)
            closeButtonPressed();
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &processor)
        {
            bool pluginStillExists = false;
            for (int i = 0; i < processor.getNumPlugins(); ++i)
            {
                if (processor.getPlugin(i) == &ownerPlugin)
                {
                    pluginStillExists = true;
                    break;
                }
            }
            if (!pluginStillExists)
                closeButtonPressed();
        }
    }

private:
    ProcessorBase& processor;
    juce::AudioPluginInstance& ownerPlugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};