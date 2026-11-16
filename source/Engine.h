#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Tonewheel.h"
#include "Combo.h"
#include "Effects.h"

namespace prisma
{
struct EngineParams
{
    float masterDb = -6.0f;
    int octave = 0;
    float twLevel = 0.8f, cbLevel = 0.0f;
    TonewheelSettings tw;
    ComboSettings cb;
    bool comboThroughRotary = false;
    WahSettings wah;
    DriveSettings drive;
    RotarySettings rotary;
    EchoSettings echo;
    RoomSettings room;
};

class Engine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setParams (const EngineParams& p);
    void process (float* L, float* R, int n, const juce::MidiBuffer& midi);

    // for the editor
    float hornSpeed() const { return rotary.hornHz(); }
    float drumSpeed() const { return rotary.drumHz(); }
    int rotarySpeedOverride() const { return speedOverride; }   // -1 none, else 1 slow / 2 fast (mod wheel)

private:
    void handle (const juce::MidiMessage& m);
    void renderChunk (float* L, float* R, int n);

    double sr = 48000;
    int maxBlock = 512;
    EngineParams params;
    int lastUiSpeed = 1, speedOverride = -1;

    Tonewheel tw;
    Combo cb;
    Wah wahA, wahB;
    Drive driveA, driveB;
    Rotary rotary;
    DrumEcho echo;
    Room room;

    std::vector<float> bufTw, bufCb, busA, busB;
    Smoothed twGain, cbGain, master, expression;
    float exprTarget = 1.0f;
    bool sustain = false;
    bool sounding[128] {};
    bool sustained[128] {};
    int noteShift[128] {};
};
}
