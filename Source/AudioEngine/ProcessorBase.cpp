/*
  ==============================================================================

    ProcessorBase.cpp
    (Definitive fix with full initialization logic in setState)

  ==============================================================================
*/

#include "AudioEngine/ProcessorBase.h"
#include "../Application/Application.h" 
#include "../Data/AppState.h" // <<< NEW: Include AppState to set the dirty flag

namespace IDs
{
    const juce::Identifier PLUGIN_CHAIN("PluginChain");
    const juce::Identifier PLUGIN("Plugin");
    const juce::Identifier uid("uid");
    const juce::Identifier state("state");
    const juce::Identifier bypassed("bypassed");
}

ProcessorBase::ProcessorBase(const juce::Identifier& id)
    : processorId(id)
{
    gain.setGainDecibels(0.0f);
}

void ProcessorBase::prepare(const juce::dsp::ProcessSpec& spec)
{
    DBG("ProcessorBase::prepare() called for '" << processorId.toString()
        << "' with Sample Rate: " << spec.sampleRate
        << ", Block Size: " << spec.maximumBlockSize);

    const juce::ScopedLock sl(pluginLock);
    this->processSpec = spec;

    gain.prepare(spec);

    tempBuffer.setSize(8, spec.maximumBlockSize);

    for (auto* plugin : pluginChain)
        if (plugin != nullptr)
            plugin->prepareToPlay(spec.sampleRate, spec.maximumBlockSize);

    reset();
}

void ProcessorBase::setState(const juce::ValueTree& newState)
{
    const juce::ScopedLock sl(pluginLock);

    if (processSpec.sampleRate <= 0)
    {
        DBG("ERROR: Cannot restore plugin state for '" << processorId.toString()
            << "'. The processor has not been prepared yet (Sample Rate is 0).");
        return;
    }

    pluginChain.clear();
    pluginBypassState.clear();

    juce::ValueTree pluginChainState = newState.getChildWithName(IDs::PLUGIN_CHAIN);
    if (!pluginChainState.isValid()) return;

    for (int i = 0; i < pluginChainState.getNumChildren(); ++i)
    {
        juce::ValueTree pluginState = pluginChainState.getChild(i);
        if (!pluginState.hasType(IDs::PLUGIN)) continue;

        int uidToFind = pluginState.getProperty(IDs::uid, 0);
        if (uidToFind == 0) continue;

        auto& pluginManager = getSharedPluginManager();
        juce::PluginDescription desc;
        bool found = false;
        for (const auto& knownDesc : pluginManager.getKnownPlugins())
        {
            if (knownDesc.uniqueId == uidToFind)
            {
                desc = knownDesc;
                found = true;
                break;
            }
        }

        if (!found) continue;

        if (auto instance = pluginManager.createPluginInstance(desc, processSpec))
        {
            instance->suspendProcessing(true);

            // <<< USER'S SUGGESTION IMPLEMENTED: Full Bus Layout Negotiation >>>
            const auto pluginName = instance->getName().toLowerCase();
            const bool isPickyPlugin = pluginName.contains("auto-tune") || pluginName.contains("antares");
            bool layoutSet = false;

            if (isPickyPlugin)
            {
                DBG("Picky plugin detected on preset load: " << instance->getName() << ". Skipping explicit layout negotiation.");
                layoutSet = true;
            }
            else
            {
                if (instance->getBusCount(true) > 1)
                    instance->getBus(true, 1)->enable(false);

                const auto stereoToStereo = juce::AudioProcessor::BusesLayout{ { juce::AudioChannelSet::stereo() }, { juce::AudioChannelSet::stereo() } };
                if (instance->checkBusesLayoutSupported(stereoToStereo))
                {
                    layoutSet = instance->setBusesLayout(stereoToStereo);
                }

                if (!layoutSet)
                {
                    const auto monoToStereo = juce::AudioProcessor::BusesLayout{ { juce::AudioChannelSet::mono() },   { juce::AudioChannelSet::stereo() } };
                    if (instance->checkBusesLayoutSupported(monoToStereo))
                    {
                        layoutSet = instance->setBusesLayout(monoToStereo);
                    }
                }
            }

            if (!layoutSet)
            {
                DBG("ERROR: Plugin from preset failed layout negotiation: " << instance->getName());
                continue;
            }

            instance->prepareToPlay(processSpec.sampleRate, (int)processSpec.maximumBlockSize);
            instance->reset();

            try
            {
                auto base64State = pluginState.getProperty(IDs::state).toString();
                juce::MemoryBlock internalState;
                if (internalState.fromBase64Encoding(base64State))
                {
                    instance->setStateInformation(internalState.getData(), (int)internalState.getSize());
                }
            }
            catch (...)
            {
                DBG("CRITICAL: Plugin '" << instance->getName() << "' threw an exception while restoring state. Skipping this plugin.");
                continue;
            }

            bool bypassed = pluginState.getProperty(IDs::bypassed, false);
            if (bypassed)
                pluginBypassState.insert(pluginChain.size());

            pluginChain.add(std::move(instance));

            if (auto* addedInstance = pluginChain.getLast())
            {
                addedInstance->suspendProcessing(false);
            }
        }
    }

    sendChangeMessage();
}

juce::ValueTree ProcessorBase::getState() const
{
    const juce::ScopedLock sl(pluginLock);

    juce::ValueTree state(processorId);
    juce::ValueTree pluginChainState(IDs::PLUGIN_CHAIN);
    state.addChild(pluginChainState, -1, nullptr);

    for (int i = 0; i < pluginChain.size(); ++i)
    {
        auto* plugin = pluginChain.getUnchecked(i);
        if (plugin == nullptr) continue;

        juce::ValueTree pluginState(IDs::PLUGIN);
        pluginState.setProperty(IDs::uid, (int)plugin->getPluginDescription().uniqueId, nullptr);

        juce::MemoryBlock internalState;
        plugin->getStateInformation(internalState);
        pluginState.setProperty(IDs::state, internalState.toBase64Encoding(), nullptr);

        pluginState.setProperty(IDs::bypassed, isPluginBypassed(i), nullptr);

        pluginChainState.addChild(pluginState, -1, nullptr);
    }

    return state;
}

void ProcessorBase::process(juce::AudioBuffer<float>& buffer)
{
    auto* currentLevelPtr = levelSource.load();
    if (muted.load())
    {
        buffer.clear();
        if (currentLevelPtr != nullptr)
            currentLevelPtr->store(0.0f);
        return;
    }

    const juce::ScopedLock sl(pluginLock);
    if (!pluginChain.isEmpty())
    {
        juce::MidiBuffer emptyMidi;
        for (int i = 0; i < pluginChain.size(); ++i)
        {
            if (isPluginBypassed(i))
                continue;

            if (auto* plugin = pluginChain.getUnchecked(i))
            {
                try
                {
                    const int requiredInputs = plugin->getTotalNumInputChannels();
                    const int requiredOutputs = plugin->getTotalNumOutputChannels();
                    const int hostChannels = buffer.getNumChannels();

                    if (requiredInputs != hostChannels || requiredOutputs != hostChannels)
                    {
                        tempBuffer.setSize(juce::jmax(requiredInputs, requiredOutputs), buffer.getNumSamples(), false, true, true);
                        tempBuffer.clear();

                        int chansToCopyIn = juce::jmin(hostChannels, requiredInputs);
                        for (int ch = 0; ch < chansToCopyIn; ++ch)
                            tempBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

                        plugin->processBlock(tempBuffer, emptyMidi);

                        buffer.clear();
                        int chansToCopyOut = juce::jmin(hostChannels, requiredOutputs);
                        for (int ch = 0; ch < chansToCopyOut; ++ch)
                            buffer.copyFrom(ch, 0, tempBuffer, ch, 0, buffer.getNumSamples());
                    }
                    else
                    {
                        plugin->processBlock(buffer, emptyMidi);
                    }
                }
                catch (...)
                {
                    DBG("!!! Unknown exception caught while processing plugin: " << plugin->getName());
                }
            }
        }
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gain.process(context);

    if (currentLevelPtr != nullptr)
    {
        float rmsLeft = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        float rmsRight = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : rmsLeft;
        currentLevelPtr->store(juce::jmax(rmsLeft, rmsRight));
    }
}

void ProcessorBase::addPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin)
{
    if (newPlugin == nullptr) return;

    const juce::ScopedLock sl(pluginLock);

    if (processSpec.sampleRate > 0)
    {
        newPlugin->suspendProcessing(true);

        const auto pluginName = newPlugin->getName().toLowerCase();
        const bool isPickyPlugin = pluginName.contains("auto-tune") || pluginName.contains("antares");

        bool layoutSet = false;

        if (isPickyPlugin)
        {
            DBG("Picky plugin detected: " << newPlugin->getName() << ". Skipping layout negotiation.");
            layoutSet = true;
        }
        else
        {
            if (newPlugin->getBusCount(true) > 1)
                newPlugin->getBus(true, 1)->enable(false);

            const auto stereoToStereo = juce::AudioProcessor::BusesLayout{ { juce::AudioChannelSet::stereo() }, { juce::AudioChannelSet::stereo() } };
            const auto monoToStereo = juce::AudioProcessor::BusesLayout{ { juce::AudioChannelSet::mono() },   { juce::AudioChannelSet::stereo() } };

            if (newPlugin->checkBusesLayoutSupported(stereoToStereo))
                layoutSet = newPlugin->setBusesLayout(stereoToStereo);

            if (!layoutSet && newPlugin->checkBusesLayoutSupported(monoToStereo))
                layoutSet = newPlugin->setBusesLayout(monoToStereo);
        }

        if (!layoutSet)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Plugin Load Failed",
                "The selected plugin has an I/O configuration that is not compatible with this application: " + newPlugin->getName());

            newPlugin->suspendProcessing(false);
            return;
        }

        newPlugin->prepareToPlay(processSpec.sampleRate, (int)processSpec.maximumBlockSize);
        newPlugin->reset();
        newPlugin->suspendProcessing(false);
    }

    pluginChain.add(std::move(newPlugin));

    // <<< FIX >>>
    AppState::getInstance().setPresetDirty(true);
    sendChangeMessage();
}

void ProcessorBase::reset()
{
    const juce::ScopedLock sl(pluginLock);
    gain.reset();
    for (auto* plugin : pluginChain)
        plugin->reset();
}

void ProcessorBase::removePlugin(int index)
{
    const juce::ScopedLock sl(pluginLock);
    if (isPositiveAndBelow(index, pluginChain.size()))
    {
        pluginBypassState.erase(index);
        pluginChain.remove(index);

        // <<< FIX >>>
        AppState::getInstance().setPresetDirty(true);
        sendChangeMessage();
    }
}

void ProcessorBase::movePlugin(int oldIndex, int newIndex)
{
    const juce::ScopedLock sl(pluginLock);
    if (isPositiveAndBelow(oldIndex, getNumPlugins()) && isPositiveAndBelow(newIndex, getNumPlugins()))
    {
        bool oldIndexBypassed = pluginBypassState.count(oldIndex);
        bool newIndexBypassed = pluginBypassState.count(newIndex);
        if (oldIndexBypassed) pluginBypassState.erase(oldIndex);
        if (newIndexBypassed) pluginBypassState.erase(newIndex);
        if (oldIndexBypassed) pluginBypassState.insert(newIndex);
        if (newIndexBypassed) pluginBypassState.insert(oldIndex);
        pluginChain.move(oldIndex, newIndex);

        // <<< FIX >>>
        AppState::getInstance().setPresetDirty(true);
        sendChangeMessage();
    }
}

void ProcessorBase::setPluginBypassed(int pluginIndex, bool shouldBeBypassed)
{
    const juce::ScopedLock sl(pluginLock);
    if (auto* plugin = pluginChain[pluginIndex])
    {
        if (auto* bypassParam = plugin->getBypassParameter())
        {
            if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(bypassParam))
            {
                boolParam->beginChangeGesture();
                boolParam->setValueNotifyingHost(shouldBeBypassed ? 1.0f : 0.0f);
                boolParam->endChangeGesture();
                pluginBypassState.erase(pluginIndex);
                // The dirty flag will be set by the audioProcessorParameterChanged callback
                return;
            }
        }
        if (shouldBeBypassed)
            pluginBypassState.insert(pluginIndex);
        else
            pluginBypassState.erase(pluginIndex);

        AppState::getInstance().setPresetDirty(true);
    }
}

bool ProcessorBase::isPluginBypassed(int index) const
{
    if (auto* plugin = pluginChain[index])
    {
        if (auto* bypassParam = plugin->getBypassParameter())
        {
            if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(bypassParam))
                return boolParam->get();
        }
        return pluginBypassState.count(index) > 0;
    }
    return false;
}

juce::AudioPluginInstance* ProcessorBase::getPlugin(int index) const
{
    const juce::ScopedLock sl(pluginLock);
    return pluginChain[index];
}

int ProcessorBase::getNumPlugins() const
{
    const juce::ScopedLock sl(pluginLock);
    return pluginChain.size();
}

void ProcessorBase::setGain(float gainInDecibels) { gain.setGainDecibels(gainInDecibels); }
float ProcessorBase::getGain() const { return gain.getGainDecibels(); }
void ProcessorBase::setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
bool ProcessorBase::isMuted() const { return muted.load(); }

void ProcessorBase::setLevelSource(std::atomic<float>* newLevelSource)
{
    levelSource.store(newLevelSource);
}