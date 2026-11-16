#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"
#include "ParamTable.h"
#include "Params.h"

class PrismaProcessor : public juce::AudioProcessor
{
public:
    PrismaProcessor();
    ~PrismaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Prisma"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // editor helpers (message thread)
    float getHornSpeed() const { return hornSpeed.load(); }
    float getDrumSpeed() const { return drumSpeed.load(); }
    int getRotaryOverride() const { return rotaryOverride.load(); }

private:
    prisma::EngineParams readParams() const;

    prisma::Engine engine;
    std::vector<std::pair<std::string, std::atomic<float>*>> raw;
    std::unordered_map<std::string, std::atomic<float>*> rawById;
    int currentProgram = 0;
    std::atomic<float> hornSpeed { 0 }, drumSpeed { 0 };
    std::atomic<int> rotaryOverride { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaProcessor)
};
