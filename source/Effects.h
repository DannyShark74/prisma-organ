#pragma once
#include "DspUtil.h"
#include <memory>

namespace juce::dsp { template <typename T> class Oversampling; }

namespace prisma
{
// ---------------------------------------------------------------- wah
struct WahSettings { bool on = false; int mode = 0; float pos = 0.5f, res = 0.6f, rate = 0.5f; };   // mode 0 pedal, 1 LFO, 2 envelope

class Wah
{
public:
    void prepare (double sampleRate);
    void reset();
    void setSettings (const WahSettings& s) { set = s; onAmt.set (s.on ? 1.0f : 0.0f); }
    void process (float* x, int n);

private:
    double sr = 48000;
    WahSettings set;
    Svf svf;
    double lfo = 0;
    float env = 0, envAtt = 0, envRel = 0, posSm = 0.5f, posCoef = 0.01f;
    Smoothed onAmt;
    int counter = 0;
};

// ---------------------------------------------------------------- tube preamp
struct DriveSettings { float drive = 0.15f, tone = 0.5f; };

class Drive
{
public:
    Drive();
    ~Drive();
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const DriveSettings& s);
    void process (float* x, int n);

private:
    double sr = 48000;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    Smoothed gain, makeup;
    float env = 0, envCoef = 0.001f;
    Biquad pre, post;
    OnePole dcBlock;
};

// ---------------------------------------------------------------- rotary speaker
struct RotarySettings { bool on = true; int speed = 1; int cab = 0; float balance = 0, width = 0.8f, inertia = 0.5f; };   // speed 0 stop, 1 slow, 2 fast

class Rotary
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const RotarySettings& s);
    void process (const float* in, float* outL, float* outR, int n);
    float hornHz() const { return hornSpeed; }
    float drumHz() const { return drumSpeed; }

private:
    void updateCab();
    double sr = 48000;
    RotarySettings set;
    int cabBuilt = -1;
    Biquad xlp1, xlp2, xhp1, xhp2;
    DelayLine horn, drum;
    double hornAng = 0, drumAng = 0.37;
    float hornSpeed = 0.83f, drumSpeed = 0.67f;
    float hornLpLo = 0, hornLpHi = 0;
    float hz[4] {};          // one-pole states for horn direct / reflection, both mics
    Biquad eqLo[2], eqHi[2], eqTop[2];
    Smoothed onAmt;
    float hornR = 0.16f, drumR = 0.12f;
};

// ---------------------------------------------------------------- magnetic drum echo
struct EchoSettings
{
    bool on = false;
    int mode = 1;              // 0 echo, 1 repeat, 2 swell
    bool head[4] { true, false, true, false };
    float timeMs = 75.0f;      // head spacing
    float feedback = 0.4f, tone = 0.5f, wear = 0.3f, spread = 0.5f, mix = 0.35f;
};

class DrumEcho
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const EchoSettings& s);
    void process (float* L, float* R, int n);

private:
    double sr = 48000;
    EchoSettings set;
    DelayLine line;
    Smoothed spacing, onAmt, fb, mix;
    OnePole fbLp, fbHp, outLpL, outLpR;
    double wow = 0;
    float flutter = 0, flutterTarget = 0;
    int flutterCount = 0;
    Rng rng { 31337u };
};

// ---------------------------------------------------------------- small room
struct RoomSettings { float size = 0.4f, mix = 0.15f; };

class Room
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const RoomSettings& s);
    void process (float* L, float* R, int n);

private:
    void build();
    double sr = 48000;
    RoomSettings set;
    float builtSize = -1;
    static constexpr int N = 8;
    DelayLine lines[N];
    DelayLine pre;
    float len[N] {}, g[N] {};
    OnePole damp[N];
    OnePole wetHpL, wetHpR;
    Smoothed mix;
};
}
