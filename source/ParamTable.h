#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "Engine.h"

namespace prisma
{
// One table describes every parameter; the plugin builds its parameter layout from it and the
// offline render tool uses the same defaults and the same mapping to engine settings.
struct ParamDef
{
    enum Kind { Float, Int, Bool, Choice };
    std::string id, name;
    Kind kind;
    float min, max, def;
    std::vector<std::string> choices {};
    float skewCentre = 0.0f;   // > 0: skewed float range centred here
    std::string unit {};
};

const std::vector<ParamDef>& paramTable();

using ParamGetter = std::function<float (const std::string&)>;
EngineParams engineParamsFrom (const ParamGetter& get);

struct Preset
{
    std::string name, category;
    std::map<std::string, float> values;   // everything else keeps its default
};
const std::vector<Preset>& factoryPresets();
}
