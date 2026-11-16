#pragma once
#include "DspUtil.h"

namespace prisma
{
struct TonewheelSettings
{
    float drawbar[9] { 8, 8, 8, 0, 0, 0, 0, 0, 0 };   // 0..8 (8 = fully out)
    bool percOn = false, percThird = false, percFast = true, percSoft = false;
    int vibMode = 6;          // 0 off, 1..3 = V1..V3, 4..6 = C1..C3
    float click = 0.5f;       // 0..1
    float leak = 0.3f;        // 0..1
    bool deep = false;        // lowest-octave 16' without fold-back
};

// Tonewheel organ: 91 always-spinning wheels (real gear ratios), 9 busbars with key contacts,
// single-trigger percussion, leakage / crosstalk, and the line-box vibrato / chorus scanner.
class Tonewheel
{
public:
    static constexpr int kWheels = 91;

    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const TonewheelSettings& s);

    void noteOn (int midi);
    void noteOff (int midi);
    void allNotesOff();
    int heldCount() const { return held; }

    // writes n mono samples (scanner included)
    void render (float* out, int n);

    static double wheelFrequency (int wheel);   // 1..91
    int wheelFor (int midi, int bar) const;     // public for tests

private:
    struct Key
    {
        int midi = -1;
        bool down = false;
        int wheel[9] {};
        float g[9] {};          // contact gain 0..1
        float target[9] {};
        int wait[9] {};         // samples until this contact moves to target
        bool active() const { return midi >= 0; }
    };

    void startContacts (Key& k, bool closing);
    float scanner (float x);

    double sr = 48000.0;
    TonewheelSettings set;

    double phase[kWheels + 1] {};
    double inc[kWheels + 1] {};
    float wheelGain[kWheels + 1] {};
    float h2[kWheels + 1] {}, h3[kWheels + 1] {};
    float leakGain[kWheels + 1] {};
    float amp[kWheels + 1] {};

    static constexpr int kMaxKeys = 128;
    Key keys[kMaxKeys];
    int held = 0;

    Smoothed bar[9];
    float rampStep = 0.01f;
    Rng rng { 12345u };

    // percussion
    float percEnv = 0, percDecay = 0.9999f;

    // key click noise
    float clickEnv = 0, clickDecay = 0.99f;
    Svf clickFilter;
    Rng noise { 777u };

    // leakage envelope (follows whether keys are down)
    float leakEnv = 0, leakAtt = 0.001f, leakRel = 0.0001f;

    // scanner
    DelayLine line;
    double scanPhase = 0, scanInc = 0;
    OnePole lineLp;
    Smoothed vibDepth, chorusMix, vibOn;

    // output colour: the tonewheel generator's own filtering and the preamp input transformer
    Biquad lowCut, highTaper;
};
}
