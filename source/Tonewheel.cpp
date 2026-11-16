#include "Tonewheel.h"

namespace prisma
{
namespace
{
    // driving / driven gear teeth for the 12 semitone positions (C..B) of the classic generator
    constexpr double kRatio[12] = { 85.0 / 104, 71.0 / 82, 67.0 / 73, 105.0 / 108, 103.0 / 100, 84.0 / 77,
                                    74.0 / 64, 98.0 / 80, 96.0 / 74, 88.0 / 64, 67.0 / 46, 108.0 / 70 };
    constexpr double kShaftHz = 20.0;   // 1200 rpm synchronous motor

    // busbar offsets in semitones relative to the 8' wheel: 16, 5 1/3, 8, 4, 2 2/3, 2, 1 3/5, 1 1/3, 1
    constexpr int kBarOffset[9] = { -12, 7, 0, 12, 19, 24, 28, 31, 36 };

    // vibrato scanner peak delay swing in ms for V1, V2, V3
    constexpr float kScanDepthMs[3] = { 0.17f, 0.34f, 0.62f };
    constexpr float kScanBaseMs = 1.0f;
    constexpr double kScanHz = 6.86;
}

double Tonewheel::wheelFrequency (int w)
{
    if (w <= 84)
    {
        const int note = (w - 1) % 12, oct = (w - 1) / 12;
        return kShaftHz * kRatio[note] * (double) (2 << oct);
    }
    // top 7 wheels have 192 teeth and use the gears a fifth below
    return kShaftHz * kRatio[(w - 85) + 5] * 192.0;
}

int Tonewheel::wheelFor (int midi, int b) const
{
    const int base8 = midi - 23;          // MIDI 36 (lowest manual C) = wheel 13 at 8'
    int w = base8 + kBarOffset[b];
    if (b == 0 && ! set.deep && w < 13 && base8 >= 13) w += 12;   // manual 16' folds back in the lowest octave
    while (w < 1) w += 12;
    while (w > kWheels) w -= 12;
    return w;
}

void Tonewheel::prepare (double sampleRate, int)
{
    sr = sampleRate;
    Rng r (4242u);
    for (int w = 1; w <= kWheels; ++w)
    {
        inc[w] = wheelFrequency (w) / sr;
        phase[w] = r.uni();   // wheels are never in phase with each other
        float db = r.bi() * 1.0f;
        if (w <= 12) db -= 1.5f;
        if (w > 60) db -= (float) (w - 60) * 0.12f;
        wheelGain[w] = dbToGain (db);
        if (w <= 12) { h3[w] = 0.12f; h2[w] = 0.03f; }
        else if (w <= 60) { h2[w] = 0.015f * r.uni(); h3[w] = (0.01f + 0.02f * r.uni()) * (float) (60 - w) / 48.0f; }
        else { h2[w] = 0.0f; h3[w] = 0.0f; }
        // keep harmonics below Nyquist
        if (wheelFrequency (w) * 3.0 > sr * 0.45) h3[w] = 0.0f;
        if (wheelFrequency (w) * 2.0 > sr * 0.45) h2[w] = 0.0f;
        leakGain[w] = 0.2f + 0.8f * r.uni();
    }
    line.prepare ((int) (sr * 0.004) + 8);
    lineLp.set (sr, 7500.0);
    scanInc = kScanHz / sr;
    clickFilter.set (sr, 3000.0, 0.8);
    clickDecay = (float) std::exp (-1.0 / (0.0025 * sr));
    leakAtt = (float) (1.0 - std::exp (-1.0 / (0.03 * sr)));
    leakRel = (float) (1.0 - std::exp (-1.0 / (0.4 * sr)));
    lowCut.highpass (sr, 28.0, 0.6);
    highTaper.lowpass (sr, std::min (11000.0, sr * 0.45), 0.6);
    const int sm = (int) (sr * 0.02);
    vibDepth.setLength (sm); chorusMix.setLength (sm); vibOn.setLength (sm);
    for (auto& b : bar) b.snap (0);
    setSettings (set);
    for (int i = 0; i < 9; ++i) bar[i].snap (set.drawbar[i]);
    vibDepth.snap (vibDepth.target); chorusMix.snap (chorusMix.target); vibOn.snap (vibOn.target);
    reset();
}

void Tonewheel::reset()
{
    for (auto& k : keys) k = Key {};
    held = 0;
    percEnv = 0; clickEnv = 0; leakEnv = 0;
    line.reset(); lineLp.reset(); clickFilter.reset();
    lowCut.reset(); highTaper.reset();
}

void Tonewheel::setSettings (const TonewheelSettings& s)
{
    set = s;
    for (int i = 0; i < 9; ++i) bar[i].target = std::clamp (s.drawbar[i], 0.0f, 8.0f);
    const float rampMs = 4.0f * std::pow (0.0125f, std::clamp (s.click, 0.0f, 1.0f));
    rampStep = 1.0f / std::max (1.0f, rampMs * 0.001f * (float) sr);
    const double tau = s.percFast ? 0.13 : 0.45;
    percDecay = (float) std::exp (-1.0 / (tau * sr));

    const int m = std::clamp (s.vibMode, 0, 6);
    vibOn.set (m == 0 ? 0.0f : 1.0f);
    if (m > 0)
    {
        vibDepth.set (kScanDepthMs[(m - 1) % 3]);
        chorusMix.set (m >= 4 ? 0.5f : 0.0f);
    }
}

void Tonewheel::startContacts (Key& k, bool closing)
{
    const float c = std::clamp (set.click, 0.0f, 1.0f);
    const float spreadMs = closing ? (0.3f + 3.0f * c) : (0.2f + 1.5f * c);
    for (int i = 0; i < 9; ++i)
    {
        k.target[i] = closing ? 1.0f : 0.0f;
        k.wait[i] = (int) (rng.uni() * spreadMs * 0.001f * (float) sr);
    }
    clickEnv = std::min (1.5f, clickEnv + 1.0f);
}

void Tonewheel::noteOn (int midi)
{
    if (midi < 0 || midi >= kMaxKeys) return;
    Key& k = keys[midi];
    if (k.down) return;
    if (held == 0) percEnv = 1.0f;   // single trigger: only when no other key is down
    k.midi = midi;
    k.down = true;
    for (int i = 0; i < 9; ++i) k.wheel[i] = wheelFor (midi, i);
    ++held;
    startContacts (k, true);
}

void Tonewheel::noteOff (int midi)
{
    if (midi < 0 || midi >= kMaxKeys) return;
    Key& k = keys[midi];
    if (! k.down) return;
    k.down = false;
    held = std::max (0, held - 1);
    startContacts (k, false);
}

void Tonewheel::allNotesOff()
{
    for (int m = 0; m < kMaxKeys; ++m) noteOff (m);
}

float Tonewheel::scanner (float x)
{
    line.push (x);
    const float on = vibOn.next(), depth = vibDepth.next(), cm = chorusMix.next();
    if (on <= 0.0f) return x;
    scanPhase += scanInc;
    if (scanPhase >= 1.0) scanPhase -= 1.0;
    // the scanner sweeps the line taps back and forth: a slightly rounded triangle
    const float tri = 4.0f * std::abs ((float) scanPhase - 0.5f) - 1.0f;
    const float shape = 0.85f * tri + 0.15f * SineTable::get() (scanPhase + 0.25 >= 1.0 ? scanPhase - 0.75 : scanPhase + 0.25);
    const float dMs = kScanBaseMs + depth * shape;
    const float vib = lineLp.lp (line.read (dMs * 0.001f * (float) sr));
    const float delayedDry = line.read (kScanBaseMs * 0.001f * (float) sr);
    const float wet = (1.0f - cm) * vib + cm * delayedDry * 1.05f + cm * 0.05f * vib;
    return x + on * (wet - x);
}

void Tonewheel::render (float* out, int n)
{
    const auto& sine = SineTable::get();
    const float barMaster = (set.percOn && ! set.percSoft) ? 0.708f : 1.0f;
    const float percLevel = set.percSoft ? 0.5f : 1.0f;
    const int percBar = set.percThird ? 4 : 3;
    const float leakAmt = std::clamp (set.leak, 0.0f, 1.0f);
    const float clickAmt = std::pow (std::clamp (set.click, 0.0f, 1.0f), 1.5f);

    // collect active keys once per call
    int active[kMaxKeys];
    int na = 0;
    for (int m = 0; m < kMaxKeys; ++m)
        if (keys[m].active()) active[na++] = m;

    int done = 0;
    while (done < n)
    {
        const int len = std::min (32, n - done);
        float barGain[9];
        for (int i = 0; i < 9; ++i)
        {
            auto& b = bar[i];
            b.cur += (b.target - b.cur) * 0.2f;
            if (std::abs (b.target - b.cur) < 0.001f) b.cur = b.target;
            const float L = b.cur;
            float g = L <= 0.0f ? 0.0f : std::exp (-0.3454f * (8.0f - std::max (1.0f, L)));   // 3 dB per step
            if (L < 1.0f) g *= L;
            barGain[i] = g * barMaster;
        }
        if (set.percOn) barGain[8] = 0.0f;   // percussion borrows the 1' busbar

        for (int s = 0; s < len; ++s)
        {
            std::fill (amp, amp + kWheels + 1, 0.0f);
            for (int a = 0; a < na; ++a)
            {
                Key& k = keys[active[a]];
                if (! k.active()) continue;
                bool any = false;
                for (int i = 0; i < 9; ++i)
                {
                    if (k.wait[i] > 0) --k.wait[i];
                    else if (k.g[i] != k.target[i])
                        k.g[i] = k.target[i] > k.g[i] ? std::min (k.target[i], k.g[i] + rampStep)
                                                        : std::max (k.target[i], k.g[i] - rampStep);
                    if (k.g[i] > 0.0f)
                    {
                        any = true;
                        amp[k.wheel[i]] += barGain[i] * k.g[i];
                    }
                    else if (k.target[i] > 0.0f) any = true;
                }
                if (set.percOn && percEnv > 1.0e-5f)
                    amp[k.wheel[percBar]] += percLevel * percEnv * k.g[8];
                if (! any && ! k.down) k.midi = -1;
            }
            percEnv *= percDecay;
            leakEnv += ((held > 0 ? 1.0f : 0.0f) - leakEnv) * (held > 0 ? leakAtt : leakRel);

            float sum = 0.0f;
            const float leakFloor = leakAmt * 0.006f * leakEnv;
            const float xtalk = leakAmt * 0.02f;
            for (int w = 1; w <= kWheels; ++w)
            {
                double& p = phase[w];
                p += inc[w];
                if (p >= 1.0) p -= 1.0;
                float a = amp[w] * wheelGain[w];
                if (leakAmt > 0.0f)
                {
                    a += leakFloor * leakGain[w];
                    if (w > 12) a += xtalk * amp[w - 12];
                    if (w <= kWheels - 12) a += xtalk * amp[w + 12];
                }
                if (a < 1.0e-7f) continue;
                float v = sine (p);
                if (h2[w] > 0.0f) { const double p2 = p * 2.0; v += h2[w] * sine (p2 - (int) p2); }
                if (h3[w] > 0.0f) { const double p3 = p * 3.0; v += h3[w] * sine (p3 - (int) p3); }
                sum += a * v;
            }

            // key click: the hard contact steps above plus a short burst of contact noise
            if (clickEnv > 1.0e-4f)
            {
                clickFilter.tick (noise.bi());
                sum += clickAmt * 0.06f * clickEnv * clickFilter.bp;
                clickEnv *= clickDecay;
            }

            float y = highTaper.tick (lowCut.tick (sum * 0.1f));
            out[done + s] = scanner (y);
        }
        done += len;

        // refresh the active list (keys may have finished releasing, none are added inside render)
        int nn = 0;
        for (int a = 0; a < na; ++a)
            if (keys[active[a]].active()) active[nn++] = active[a];
        na = nn;
    }
}
}
