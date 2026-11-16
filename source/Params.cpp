#include "Params.h"
#include "ParamTable.h"

namespace prisma
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : paramTable())
    {
        const juce::ParameterID id { d.id, 1 };
        switch (d.kind)
        {
            case ParamDef::Float:
            {
                juce::NormalisableRange<float> r (d.min, d.max);
                if (d.skewCentre > 0.0f) r.setSkewForCentre (d.skewCentre);
                auto attrs = juce::AudioParameterFloatAttributes().withLabel (d.unit);
                if (d.unit.empty() && d.min >= 0.0f && d.max <= 1.0f)
                    attrs = attrs.withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; });
                layout.add (std::make_unique<juce::AudioParameterFloat> (id, d.name, r, d.def, attrs));
                break;
            }
            case ParamDef::Int:
                layout.add (std::make_unique<juce::AudioParameterInt> (id, d.name, (int) d.min, (int) d.max, (int) d.def));
                break;
            case ParamDef::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (id, d.name, d.def >= 0.5f));
                break;
            case ParamDef::Choice:
            {
                juce::StringArray ch;
                for (auto& c : d.choices) ch.add (c);
                layout.add (std::make_unique<juce::AudioParameterChoice> (id, d.name, ch, (int) d.def));
                break;
            }
        }
    }
    layout.add (std::make_unique<PresetInfoParam>());   // last, so the indexes of all earlier parameters stay as they were
    return layout;
}
}
