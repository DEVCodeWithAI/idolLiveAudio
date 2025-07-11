/*
  ==============================================================================

    ProcessorBase.cpp
    (Definitive fix with full initialization logic in setState and safe removal)

  ==============================================================================
*/

#include "AudioEngine/ProcessorBase.h"
#include "../Application/Application.h" 
#include "../Data/AppState.h"

// <<< ADDED: Identifiers for new properties >>>
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
    : processorId(id)
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

// <<< MODIFIED: Replaced sleep() with a more robust suspend/resume logic >>>
void ProcessorBase::setState(const juce::ValueTree& newState)
{
    const juce::ScopedLock sl(pluginLock);

    if (processSpec.sampleRate <= 0)
    {
        DBG("ERROR: Cannot restore plugin state for '" << processorId.toString()
            << "'. The processor has not been prepared yet (Sample Rate is 0).");
        return;
    }

    setSendLevel(newState.getProperty(IDs::sendLevel, 1.0f));
    setReturnLevel(newState.getProperty(IDs::returnLevel, 1.0f));

    pluginChain.clear();
    pluginBypassState.clear();

    juce::ValueTree pluginChainState = newState.getChildWithName(IDs::PLUGIN_CHAIN);
    if (!pluginChainState.isValid())
    {
        sendChangeMessage(); // Gửi thông báo để UI cập nhật dù không có plugin
        return;
    }

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

        if (!found)
        {
            DBG("Plugin with UID " + juce::String(uidToFind) + " not found in known plugins list. Skipping.");
            continue;
        }

        DBG("Loading plugin " + juce::String(i + 1) + "/" + juce::String(pluginChainState.getNumChildren()) + ": " + desc.name);
        if (auto instance = pluginManager.createPluginInstance(desc, processSpec))
        {
            // <<< LOGIC MỚI: Tạm dừng plugin trước khi cấu hình >>>
            instance->suspendProcessing(true);

            instance->prepareToPlay(processSpec.sampleRate, (int)processSpec.maximumBlockSize);

            DBG("  - Restoring state for: " + instance->getName());
            try
            {
                auto base64State = pluginState.getProperty(IDs::state).toString();
                juce::MemoryBlock internalState;
                if (internalState.fromBase64Encoding(base64State))
                    instance->setStateInformation(internalState.getData(), (int)internalState.getSize());
            }
            catch (...)
            {
                DBG("  - CRITICAL: Plugin '" << instance->getName() << "' threw an exception while restoring state. Skipping this plugin.");
                continue; // Bỏ qua plugin này và tiếp tục với plugin tiếp theo
            }

            // <<< LOGIC MỚI: Kích hoạt lại plugin sau khi đã cấu hình xong >>>
            instance->suspendProcessing(false);

            bool bypassed = pluginState.getProperty(IDs::bypassed, false);
            if (bypassed)
                pluginBypassState.insert(pluginChain.size());

            pluginChain.add(std::move(instance));
            DBG("  - Successfully loaded and added to chain.");
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
    for (auto* plugin : pluginChain)
        plugin->reset();
}

void ProcessorBase::removePlugin(int index)
{
    const juce::ScopedLock sl(pluginLock);
    if (!isPositiveAndBelow(index, pluginChain.size()))
        return;

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

void ProcessorBase::setLevelSource(std::atomic<float>* newLevelSource)
{
    levelSource.store(newLevelSource);
}

void ProcessorBase::setSendLevel(float newLevel0To1) { sendLevel.store(newLevel0To1); }
float ProcessorBase::getSendLevel() const { return sendLevel.load(); }
void ProcessorBase::setReturnLevel(float newLevel0To1) { returnLevel.store(newLevel0To1); }
float ProcessorBase::getReturnLevel() const { return returnLevel.load(); }