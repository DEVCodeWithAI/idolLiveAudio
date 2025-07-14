/*
  ==============================================================================

    ProcessorBase.cpp
    (Fixed: Implemented robust, two-stage preset loading)

  ==============================================================================
*/

#include "AudioEngine/ProcessorBase.h"
#include "../Application/Application.h" 
#include "../Data/AppState.h"

namespace IDs
{
    const juce::Identifier PLUGIN_CHAIN("PluginChain");
    const juce::Identifier PLUGIN("Plugin");
    const juce::Identifier uid("uid");
    const juce::Identifier state("state");
    const juce::Identifier bypassed("bypassed");
    const juce::Identifier sendLevel("sendLevel");
    const juce::Identifier returnLevel("returnLevel");
}

ProcessorBase::ProcessorBase(const juce::Identifier& id)
    : processorId(id),
    processSpec{ 0.0, 0, 0 }
{
    gain.setGainDecibels(0.0f);
}

ProcessorBase::~ProcessorBase()
{
    const juce::ScopedLock sl(pluginLock);
    DBG("ProcessorBase Destructor called for '" << processorId.toString() << "'");

    for (int i = pluginChain.size(); --i >= 0;)
    {
        if (auto* plugin = pluginChain.getUnchecked(i))
        {
            try
            {
                DBG("Suspending plugin before destruction: " << plugin->getName());
                plugin->suspendProcessing(true);
                const auto name = plugin->getName().toLowerCase();
                if (name.contains("waves") || name.contains("wave"))
                {
                    DBG("Detected Waves plugin. Sleeping for safe shutdown...");
                    juce::Thread::sleep(500);
                }
            }
            catch (...)
            {
                DBG("Exception during plugin suspend in destructor.");
            }
        }
    }
    pluginChain.clear();
}

void ProcessorBase::prepare(const juce::dsp::ProcessSpec& spec)
{
    const juce::ScopedLock sl(pluginLock);
    this->processSpec = spec;
    gain.prepare(spec);
    tempBuffer.setSize(8, spec.maximumBlockSize);

    for (auto* plugin : pluginChain)
        if (plugin != nullptr)
            plugin->prepareToPlay(spec.sampleRate, spec.maximumBlockSize);

    reset();
}

void ProcessorBase::setState(const juce::ValueTree& processorState)
{
    const juce::ScopedLock sl(pluginLock);

    if (processSpec.sampleRate <= 0) return;
    if (!processorState.hasType(processorId)) return;

    setSendLevel(processorState.getProperty(IDs::sendLevel, 1.0f));
    setReturnLevel(processorState.getProperty(IDs::returnLevel, 1.0f));

    pluginChain.clear();
    pluginBypassState.clear();

    juce::ValueTree pluginChainState = processorState.getChildWithName(IDs::PLUGIN_CHAIN);
    if (!pluginChainState.isValid())
    {
        sendChangeMessage();
        return;
    }

    for (int i = 0; i < pluginChainState.getNumChildren(); ++i)
    {
        juce::ValueTree pluginState = pluginChainState.getChild(i);
        if (!pluginState.hasType(IDs::PLUGIN)) continue;

        int uidToFind = pluginState.getProperty(IDs::uid, 0);
        if (uidToFind == 0) continue;

        auto& pluginManager = getSharedPluginManager();

        // <<< FIXED: Correct search logic >>>
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
        if (!found)
        {
            continue; // Plugin not found
        }

        try
        {
            if (auto instance = pluginManager.createPluginInstance(desc, processSpec))
            {
                instance->suspendProcessing(true);
                instance->prepareToPlay(processSpec.sampleRate, (int)processSpec.maximumBlockSize);
                if (pluginState.hasProperty(IDs::state))
                {
                    auto base64State = pluginState.getProperty(IDs::state).toString();
                    if (!base64State.isEmpty())
                    {
                        juce::MemoryBlock internalState;
                        if (internalState.fromBase64Encoding(base64State))
                            instance->setStateInformation(internalState.getData(), (int)internalState.getSize());
                    }
                }
                instance->suspendProcessing(false);
                if ((bool)pluginState.getProperty(IDs::bypassed, false))
                    pluginBypassState.insert(pluginChain.size());
                pluginChain.add(std::move(instance));
            }
        }
        catch (...)
        {
            DBG("Exception loading plugin in setState: " << desc.name);
        }
    }
    sendChangeMessage();
}

juce::ValueTree ProcessorBase::getState() const
{
    const juce::ScopedLock sl(pluginLock);
    juce::ValueTree state(processorId);
    state.setProperty(IDs::sendLevel, getSendLevel(), nullptr);
    state.setProperty(IDs::returnLevel, getReturnLevel(), nullptr);

    juce::ValueTree pluginChainState(IDs::PLUGIN_CHAIN);
    state.addChild(pluginChainState, -1, nullptr);

    for (int i = 0; i < pluginChain.size(); ++i)
    {
        if (auto* plugin = pluginChain.getUnchecked(i))
        {
            juce::ValueTree pluginState(IDs::PLUGIN);
            pluginState.setProperty(IDs::uid, (int)plugin->getPluginDescription().uniqueId, nullptr);

            juce::MemoryBlock internalState;
            plugin->getStateInformation(internalState);
            pluginState.setProperty(IDs::state, internalState.toBase64Encoding(), nullptr);

            pluginState.setProperty(IDs::bypassed, isPluginBypassed(i), nullptr);
            pluginChainState.addChild(pluginState, -1, nullptr);
        }
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
            if (isPluginBypassed(i)) continue;
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
                catch (...) { /* ... */ }
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
        newPlugin->prepareToPlay(processSpec.sampleRate, (int)processSpec.maximumBlockSize);
        newPlugin->reset();
    }
    pluginChain.add(std::move(newPlugin));
    AppState::getInstance().setPresetDirty(true);
    sendChangeMessage();
}

void ProcessorBase::reset()
{
    const juce::ScopedLock sl(pluginLock);
    gain.reset();
    for (auto* plugin : pluginChain) plugin->reset();
}

void ProcessorBase::removePlugin(int index)
{
    const juce::ScopedLock sl(pluginLock);
    if (!isPositiveAndBelow(index, pluginChain.size())) return;
    if (auto* plugin = pluginChain.getUnchecked(index))
    {
        try
        {
            DBG("Preparing to safely remove plugin: " << plugin->getName());
            plugin->suspendProcessing(true);
            const auto name = plugin->getName().toLowerCase();
            if (name.contains("waves") || name.contains("wave"))
            {
                DBG("Detected Waves plugin. Sleeping briefly to allow thread cleanup...");
                juce::Thread::sleep(500);
            }
            if (plugin->hasEditor())
            {
                if (auto* editor = plugin->getActiveEditor())
                {
                    DBG("Closing plugin editor before removal: " << plugin->getName());
                    editor->setVisible(false);
                }
            }
        }
        catch (...)
        {
            DBG("Exception during plugin suspend or editor close before removal.");
        }
    }
    pluginBypassState.erase(index);
    pluginChain.remove(index);
    AppState::getInstance().setPresetDirty(true);
    sendChangeMessage();
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

void ProcessorBase::setLevelSource(std::atomic<float>* newLevelSource) { levelSource.store(newLevelSource); }
void ProcessorBase::setSendLevel(float newLevel0To1) { sendLevel.store(newLevel0To1); }
float ProcessorBase::getSendLevel() const { return sendLevel.load(); }
void ProcessorBase::setReturnLevel(float newLevel0To1) { returnLevel.store(newLevel0To1); }
float ProcessorBase::getReturnLevel() const { return returnLevel.load(); }

bool ProcessorBase::isPluginChainIdentical(const juce::ValueTree& newState) const
{
    const juce::ScopedLock sl(pluginLock);
    const juce::ValueTree newProcessorState = newState.getChildWithName(processorId);

    if (!newProcessorState.isValid())
        return getNumPlugins() == 0;

    const juce::ValueTree newPluginChainState = newProcessorState.getChildWithName(IDs::PLUGIN_CHAIN);

    if (getNumPlugins() != newPluginChainState.getNumChildren())
        return false;

    for (int i = 0; i < getNumPlugins(); ++i)
    {
        const auto* currentPlugin = getPlugin(i);
        const juce::ValueTree newPluginState = newPluginChainState.getChild(i);
        if (currentPlugin == nullptr || !newPluginState.isValid()) return false;
        if ((int)currentPlugin->getPluginDescription().uniqueId != (int)newPluginState.getProperty(IDs::uid, 0))
            return false;
    }
    return true;
}

bool ProcessorBase::tryHotSwapState(const juce::ValueTree& newState)
{
    const juce::ScopedLock sl(pluginLock);
    const juce::ValueTree newProcessorState = newState.getChildWithName(processorId);
    if (!newProcessorState.isValid()) return true;

    const juce::ValueTree newPluginChainState = newProcessorState.getChildWithName(IDs::PLUGIN_CHAIN);
    if (!newPluginChainState.isValid()) return true;
    jassert(getNumPlugins() == newPluginChainState.getNumChildren());
    setSendLevel(newProcessorState.getProperty(IDs::sendLevel, 1.0f));
    setReturnLevel(newProcessorState.getProperty(IDs::returnLevel, 1.0f));

    for (int i = 0; i < getNumPlugins(); ++i)
    {
        auto* plugin = getPlugin(i);
        const juce::ValueTree newPluginState = newPluginChainState.getChild(i);
        try
        {
            if (newPluginState.hasProperty(IDs::state))
            {
                auto base64State = newPluginState.getProperty(IDs::state).toString();
                if (!base64State.isEmpty())
                {
                    juce::MemoryBlock internalState;
                    if (internalState.fromBase64Encoding(base64State))
                        plugin->setStateInformation(internalState.getData(), (int)internalState.getSize());
                }
            }
            setPluginBypassed(i, newPluginState.getProperty(IDs::bypassed, false));
        }
        catch (...)
        {
            DBG("HOT-SWAP FAILED for plugin '" << plugin->getName() << "'.");
            return false;
        }
    }
    sendChangeMessage();
    return true;
}

// ==============================================================================
// <<< IMPLEMENTATION of the new robust loading methods >>>
// ==============================================================================

bool ProcessorBase::prepareToLoadState(const juce::ValueTree& newState)
{
    const juce::ValueTree processorState = newState.getChildWithName(processorId);

    preparedSendLevel = processorState.getProperty(IDs::sendLevel, 1.0f);
    preparedReturnLevel = processorState.getProperty(IDs::returnLevel, 1.0f);

    auto tempChain = std::make_unique<juce::OwnedArray<juce::AudioPluginInstance>>();
    auto tempBypass = std::make_unique<std::unordered_set<int>>();

    const juce::ValueTree newPluginChainState = processorState.getChildWithName(IDs::PLUGIN_CHAIN);
    if (newPluginChainState.isValid())
    {
        auto& pluginManager = getSharedPluginManager();
        for (int i = 0; i < newPluginChainState.getNumChildren(); ++i)
        {
            const juce::ValueTree pluginState = newPluginChainState.getChild(i);
            if (!pluginState.hasType(IDs::PLUGIN)) continue;

            const int uidToFind = pluginState.getProperty(IDs::uid, 0);
            if (uidToFind == 0) continue;

            // <<< FIXED: Correct search logic >>>
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
            if (!found)
            {
                DBG("Plugin with UID " << uidToFind << " not in known list. Cannot prepare.");
                return false;
            }

            try
            {
                if (auto instance = pluginManager.createPluginInstance(desc, processSpec))
                {
                    instance->suspendProcessing(true);
                    if (pluginState.hasProperty(IDs::state))
                    {
                        auto base64State = pluginState.getProperty(IDs::state).toString();
                        if (!base64State.isEmpty())
                        {
                            juce::MemoryBlock internalState;
                            if (internalState.fromBase64Encoding(base64State))
                                instance->setStateInformation(internalState.getData(), (int)internalState.getSize());
                        }
                    }
                    if ((bool)pluginState.getProperty(IDs::bypassed, false))
                        tempBypass->insert(i);
                    tempChain->add(std::move(instance));
                }
                else
                {
                    DBG("Failed to create prepared instance for: " << desc.name);
                    return false;
                }
            }
            catch (...)
            {
                DBG("Exception while preparing instance for: " << desc.name);
                return false;
            }
        }
    }

    preparedPluginChain = std::move(tempChain);
    preparedBypassState = std::move(tempBypass);
    return true;
}

void ProcessorBase::commitStateLoad()
{
    const juce::ScopedLock sl(pluginLock);

    setSendLevel(preparedSendLevel);
    setReturnLevel(preparedReturnLevel);

    if (preparedPluginChain)
        pluginChain = std::move(*preparedPluginChain);
    else
        pluginChain.clear();

    if (preparedBypassState)
        pluginBypassState = std::move(*preparedBypassState);
    else
        pluginBypassState.clear();

    preparedPluginChain.reset();
    preparedBypassState.reset();
    sendChangeMessage();
}