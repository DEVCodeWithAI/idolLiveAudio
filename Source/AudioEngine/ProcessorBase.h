/*
  ==============================================================================

    ProcessorBase.h
    (Final Cleaned Version)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Components/LevelMeter.h"
#include <unordered_set>

class ProcessorBase : public juce::ChangeBroadcaster
{
public:
    ProcessorBase(const juce::Identifier& id);
    ~ProcessorBase() override = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    void addPlugin(std::unique_ptr<juce::AudioPluginInstance> newPlugin);
    void removePlugin(int index);
    void setPluginBypassed(int pluginIndex, bool shouldBeBypassed);
    void movePlugin(int oldIndex, int newIndex);

    juce::AudioPluginInstance* getPlugin(int index) const;
    int getNumPlugins() const;

    void setGain(float gainInDecibels);
    float getGain() const;
    void setMuted(bool shouldBeMuted);
    bool isMuted() const;

    void setLevelSource(std::atomic<float>* newLevelSource);

    const juce::dsp::ProcessSpec& getProcessSpec() const { return processSpec; }
    bool isPluginBypassed(int index) const;

    juce::ValueTree getState() const;
    void setState(const juce::ValueTree& newState);

private:
    juce::Identifier processorId;
    juce::CriticalSection pluginLock;
    juce::dsp::ProcessSpec processSpec;
    juce::OwnedArray<juce::AudioPluginInstance> pluginChain;
    juce::dsp::Gain<float> gain;
    std::atomic<bool> muted = false;

    std::atomic<std::atomic<float>*> levelSource{ nullptr };

    std::unordered_set<int> pluginBypassState;

    juce::AudioBuffer<float> tempBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorBase)
};