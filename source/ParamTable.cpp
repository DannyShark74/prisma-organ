#include "ParamTable.h"
#include "Params.h"

namespace prisma
{
const std::vector<ParamDef>& paramTable()
{
    static const std::vector<ParamDef> table = []
    {
        using K = ParamDef;
        std::vector<ParamDef> t;
        auto f = [&t] (const char* id, const char* name, float mn, float mx, float d, const char* unit = "")
        { t.push_back ({ id, name, K::Float, mn, mx, d, {}, 0.0f, unit }); };
        auto b = [&t] (const char* id, const char* name, bool d) { t.push_back ({ id, name, K::Bool, 0, 1, d ? 1.0f : 0.0f }); };
        auto c = [&t] (const char* id, const char* name, std::vector<std::string> ch, int d)
        { t.push_back ({ id, name, K::Choice, 0, (float) ch.size() - 1, (float) d, ch }); };
        auto i = [&t] (const char* id, const char* name, int mn, int mx, int d) { t.push_back ({ id, name, K::Int, (float) mn, (float) mx, (float) d }); };

        f (pid::master, "Volume", -60.0f, 6.0f, -6.0f, "dB");
        i (pid::octave, "Octave", -2, 1, 0);
        f (pid::twLevel, "Tonewheel Level", 0, 1, 0.8f);
        f (pid::cbLevel, "Combo Level", 0, 1, 0.0f);

        static const char* barNames[9] = { "Drawbar 16'", "Drawbar 5 1/3'", "Drawbar 8'", "Drawbar 4'", "Drawbar 2 2/3'",
                                           "Drawbar 2'", "Drawbar 1 3/5'", "Drawbar 1 1/3'", "Drawbar 1'" };
        static const int barDef[9] = { 8, 8, 8, 0, 0, 0, 0, 0, 0 };
        for (int k = 0; k < 9; ++k) i (pid::drawbar (k), barNames[k], 0, 8, barDef[k]);
        b (pid::percOn, "Percussion", false);
        b (pid::percThird, "Percussion 3rd", false);
        b (pid::percFast, "Percussion Fast", true);
        b (pid::percSoft, "Percussion Soft", false);
        c (pid::vibMode, "Vibrato / Chorus", { "Off", "V1", "V2", "V3", "C1", "C2", "C3" }, 6);
        f (pid::click, "Key Click", 0, 1, 0.5f);
        f (pid::leak, "Leakage", 0, 1, 0.3f);
        b (pid::deep, "Deep 16'", false);

        static const bool tabDef[pid::kComboTabs] = { false, false, true, false, false, true, false, false, false, false };
        for (int k = 0; k < pid::kComboTabs; ++k)
            b (pid::comboTab (k), (std::string ("Combo ") + pid::comboTabName (k)).c_str(), tabDef[k]);
        c (pid::cbVib, "Combo Vibrato", { "Off", "Light", "Heavy" }, 1);
        b (pid::cbVibFast, "Combo Vibrato Fast", false);
        f (pid::cbTone, "Combo Tone", 0, 1, 0.5f);
        f (pid::cbAge, "Combo Age", 0, 1, 0.3f);
        b (pid::cbRotary, "Combo Through Rotary", false);

        b (pid::wahOn, "Wah", false);
        c (pid::wahMode, "Wah Mode", { "Pedal", "LFO", "Envelope" }, 0);
        f (pid::wahPos, "Wah Pedal", 0, 1, 0.5f);
        f (pid::wahRes, "Wah Resonance", 0, 1, 0.6f);
        t.push_back ({ pid::wahRate, "Wah Rate", K::Float, 0.05f, 8.0f, 0.6f, {}, 0.8f, "Hz" });

        f (pid::drive, "Drive", 0, 1, 0.15f);
        f (pid::driveTone, "Drive Tone", 0, 1, 0.5f);

        b (pid::rotOn, "Rotary", true);
        c (pid::rotSpeed, "Rotary Speed", { "Stop", "Slow", "Fast" }, 1);
        c (pid::rotCab, "Rotary Cabinet", { "Large", "Small" }, 0);
        f (pid::rotBalance, "Rotary Drum/Horn", -1, 1, 0.0f);
        f (pid::rotWidth, "Rotary Width", 0, 1, 0.8f);
        f (pid::rotInertia, "Rotary Inertia", 0, 1, 0.5f);

        b (pid::echoOn, "Echo", false);
        c (pid::echoMode, "Echo Mode", { "Echo", "Repeat", "Swell" }, 1);
        static const bool headDef[4] = { true, false, true, false };
        for (int k = 0; k < 4; ++k)
            b (pid::echoHead (k), (std::string ("Echo Head ") + std::to_string (k + 1)).c_str(), headDef[k]);
        f (pid::echoTime, "Echo Time", 40.0f, 160.0f, 75.0f, "ms");
        f (pid::echoFb, "Echo Swell", 0, 1, 0.4f);
        f (pid::echoTone, "Echo Tone", 0, 1, 0.5f);
        f (pid::echoWear, "Echo Wear", 0, 1, 0.3f);
        f (pid::echoSpread, "Echo Spread", 0, 1, 0.5f);
        f (pid::echoMix, "Echo Mix", 0, 1, 0.35f);

        f (pid::roomSize, "Room Size", 0, 1, 0.4f);
        f (pid::roomMix, "Room Mix", 0, 1, 0.15f);
        return t;
    }();
    return table;
}

EngineParams engineParamsFrom (const ParamGetter& get)
{
    auto on = [&get] (const char* id) { return get (id) >= 0.5f; };
    auto ii = [&get] (const char* id) { return (int) std::lround (get (id)); };
    EngineParams p;
    p.masterDb = get (pid::master);
    p.octave = ii (pid::octave);
    p.twLevel = get (pid::twLevel);
    p.cbLevel = get (pid::cbLevel);

    for (int k = 0; k < 9; ++k) p.tw.drawbar[k] = get (pid::drawbar (k));
    p.tw.percOn = on (pid::percOn);
    p.tw.percThird = on (pid::percThird);
    p.tw.percFast = on (pid::percFast);
    p.tw.percSoft = on (pid::percSoft);
    p.tw.vibMode = ii (pid::vibMode);
    p.tw.click = get (pid::click);
    p.tw.leak = get (pid::leak);
    p.tw.deep = on (pid::deep);

    for (int k = 0; k < pid::kComboTabs; ++k) p.cb.tab[k] = on (pid::comboTab (k));
    p.cb.vib = ii (pid::cbVib);
    p.cb.vibFast = on (pid::cbVibFast);
    p.cb.tone = get (pid::cbTone);
    p.cb.age = get (pid::cbAge);
    p.comboThroughRotary = on (pid::cbRotary);

    p.wah.on = on (pid::wahOn);
    p.wah.mode = ii (pid::wahMode);
    p.wah.pos = get (pid::wahPos);
    p.wah.res = get (pid::wahRes);
    p.wah.rate = get (pid::wahRate);

    p.drive.drive = get (pid::drive);
    p.drive.tone = get (pid::driveTone);

    p.rotary.on = on (pid::rotOn);
    p.rotary.speed = ii (pid::rotSpeed);
    p.rotary.cab = ii (pid::rotCab);
    p.rotary.balance = get (pid::rotBalance);
    p.rotary.width = get (pid::rotWidth);
    p.rotary.inertia = get (pid::rotInertia);

    p.echo.on = on (pid::echoOn);
    p.echo.mode = ii (pid::echoMode);
    for (int k = 0; k < 4; ++k) p.echo.head[k] = on (pid::echoHead (k));
    p.echo.timeMs = get (pid::echoTime);
    p.echo.feedback = get (pid::echoFb);
    p.echo.tone = get (pid::echoTone);
    p.echo.wear = get (pid::echoWear);
    p.echo.spread = get (pid::echoSpread);
    p.echo.mix = get (pid::echoMix);

    p.room.size = get (pid::roomSize);
    p.room.mix = get (pid::roomMix);
    return p;
}
}
