#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace prisma;

PrismaProcessor::PrismaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PRISMA", createLayout())
{
    for (auto& d : paramTable())
    {
        auto* p = apvts.getRawParameterValue (d.id);
        jassert (p != nullptr);
        raw.emplace_back (d.id, p);
        rawById[d.id] = p;
    }
}

bool PrismaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void PrismaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.setParams (readParams());
    engine.prepare (sampleRate, std::max (64, samplesPerBlock));
    engine.reset();
}

EngineParams PrismaProcessor::readParams() const
{
    return engineParamsFrom ([this] (const std::string& id)
    {
        auto it = rawById.find (id);
        return it != rawById.end() ? it->second->load() : 0.0f;
    });
}

void PrismaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    engine.setParams (readParams());

    if (buffer.getNumChannels() >= 2)
        engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n, midi);
    else if (buffer.getNumChannels() == 1)
    {
        juce::AudioBuffer<float> tmp (2, n);
        tmp.clear();
        engine.process (tmp.getWritePointer (0), tmp.getWritePointer (1), n, midi);
        buffer.copyFrom (0, 0, tmp, 0, 0, n);
        buffer.addFrom (0, 0, tmp, 1, 0, n);
        buffer.applyGain (0.5f);
    }
    hornSpeed.store (engine.hornSpeed());
    drumSpeed.store (engine.drumSpeed());
    rotaryOverride.store (engine.rotarySpeedOverride());
}

int PrismaProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String PrismaProcessor::getProgramName (int index)
{
    const auto& list = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) list.size()) ? juce::String (list[(size_t) index].name) : juce::String();
}

void PrismaProcessor::setCurrentProgram (int index)
{
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    currentProgram = index;
    const auto& pr = list[(size_t) index];
    for (auto& d : paramTable())
    {
        auto* param = apvts.getParameter (d.id);
        if (param == nullptr) continue;
        auto it = pr.values.find (d.id);
        const float v = it != pr.values.end() ? it->second : d.def;
        param->setValueNotifyingHost (param->convertTo0to1 (v));
    }
}

void PrismaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PrismaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* PrismaProcessor::createEditor() { return new PrismaEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PrismaProcessor(); }
