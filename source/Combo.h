#pragma once
#include "DspUtil.h"

namespace prisma
{
struct ComboSettings
{
    // Bass 16, Strings 16, Flute 8, Oboe 8, Trumpet 8, Strings 8, Flute 4, Strings 4, 2 2/3, 2
    bool tab[10] { false, false, true, false, false, true, false, false, false, false };
    int vib = 0;            // 0 off, 1 light, 2 heavy
    bool vibFast = false;
    float tone = 0.5f;      // dark .. bright
    float age = 0.3f;       // oscillator drift / mistuning
};

// Transistor combo organ in the 60s Italian style: 12 top oscillators divided down by flip-flops
// (so every octave of a note is phase locked), square waves keyed onto footage busbars,
// fixed formant filters per voice tab, pitch vibrato on the master oscillators.
class Combo
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const ComboSettings& s);
    void noteOn (int midi);
    void noteOff (int midi);
    void allNotesOff();
    void render (float* out, int n);

private:
    static constexpr int kBuses = 5;             // 16, 8, 4, 2 2/3, 2
    static constexpr int kMaxKeys = 128;
    struct Key
    {
        bool down = false;
        float env = 0.0f;
    };

    double sr = 48000.0;
    ComboSettings set;
    Key keys[kMaxKeys];

    double phase0[12] {};      // master phase per pitch class at MIDI octave 0 (dividers derive the rest)
    double baseInc[12] {};
    float detuneCents[12] {};
    double lfoPhase = 0, driftPhase = 0;
    Smoothed vibDepth;
    float attack = 0.01f, release = 0.001f;

    Svf f[14];
    Biquad lowShelf, highShelf;
    Smoothed tabGain[10];
};
}
