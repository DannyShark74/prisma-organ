#include "Combo.h"

namespace prisma
{
namespace
{
    constexpr int kBusOffset[5] = { -12, 0, 12, 19, 24 };
    constexpr int kTabBus[10] = { 0, 0, 1, 1, 1, 1, 2, 2, 3, 4 };

    inline double midiHz (double m) { return 440.0 * std::pow (2.0, (m - 69.0) / 12.0); }

    inline float polyBlep (double t, double dt)
    {
        if (t < dt) { const double x = t / dt; return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt) { const double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }
    inline double fastExp2 (double x)   // |x| small (a few % of an octave)
    {
        const double y = x * 0.69314718055994531;
        return 1.0 + y + y * y * 0.5 + y * y * y * (1.0 / 6.0);
    }
}

void Combo::prepare (double sampleRate, int)
{
    sr = sampleRate;
    Rng r (999u);
    for (int c = 0; c < 12; ++c)
    {
        baseInc[c] = midiHz (c) / sr;
        phase0[c] = r.uni();
        detuneCents[c] = r.bi() * 7.0f;
    }
    attack = (float) (1.0 / (0.0012 * sr));
    release = (float) (1.0 / (0.010 * sr));
    vibDepth.setLength ((int) (sr * 0.05));
    for (auto& g : tabGain) g.setLength ((int) (sr * 0.01));

    f[0].set (sr, 380, 0.7);  f[1].set (sr, 380, 0.7);     // bass 16
    f[2].set (sr, 850, 0.8);                               // strings 16
    f[3].set (sr, 1000, 0.6); f[4].set (sr, 1000, 0.6);    // flute 8
    f[5].set (sr, 1500, 2.0);                              // oboe 8
    f[6].set (sr, 1100, 1.0);                              // trumpet 8
    f[7].set (sr, 1800, 0.9);                              // strings 8
    f[8].set (sr, 2200, 0.6); f[9].set (sr, 2200, 0.6);    // flute 4
    f[10].set (sr, 3000, 0.9);                             // strings 4
    f[11].set (sr, 3200, 0.6);                             // 2 2/3
    f[12].set (sr, 3800, 0.6);                             // 2
    f[13].set (sr, 9000, 0.7);                             // smooth the top of the string voices
    setSettings (set);
    for (int i = 0; i < 10; ++i) tabGain[i].snap (set.tab[i] ? 1.0f : 0.0f);
    reset();
}

void Combo::reset()
{
    for (auto& k : keys) k = Key {};
    for (auto& x : f) x.reset();
    lowShelf.reset(); highShelf.reset();
}

void Combo::setSettings (const ComboSettings& s)
{
    set = s;
    for (int i = 0; i < 10; ++i) tabGain[i].set (s.tab[i] ? 1.0f : 0.0f);
    vibDepth.set (s.vib == 0 ? 0.0f : (s.vib == 1 ? 14.0f : 32.0f));
    const float t = (std::clamp (s.tone, 0.0f, 1.0f) - 0.5f) * 2.0f;
    lowShelf.lowShelf (sr, 250.0, -4.0 * t);
    highShelf.highShelf (sr, 3000.0, 6.0 * t);
}

void Combo::noteOn (int midi)
{
    if (midi >= 0 && midi < kMaxKeys) keys[midi].down = true;
}
void Combo::noteOff (int midi)
{
    if (midi >= 0 && midi < kMaxKeys) keys[midi].down = false;
}
void Combo::allNotesOff()
{
    for (auto& k : keys) k.down = false;
}

void Combo::render (float* out, int n)
{
    int active[kMaxKeys];
    int na = 0;
    for (int m = 0; m < kMaxKeys; ++m)
        if (keys[m].down || keys[m].env > 0.0f) active[na++] = m;

    const double lfoInc = (set.vibFast ? 7.2 : 5.6) / sr;
    const double driftInc = 0.07 / sr;
    const float age = std::clamp (set.age, 0.0f, 1.0f);
    const auto& sine = SineTable::get();

    for (int s = 0; s < n; ++s)
    {
        float tg[10];
        bool busUsed[kBuses] {};
        for (int i = 0; i < 10; ++i)
        {
            tg[i] = tabGain[i].next();
            if (tg[i] > 0.0f) busUsed[kTabBus[i]] = true;
        }

        lfoPhase += lfoInc; if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        driftPhase += driftInc; if (driftPhase >= 1.0) driftPhase -= 1.0;
        const float vibCents = vibDepth.next() * sine (lfoPhase);
        const float driftCents = age * 3.0f * sine (driftPhase);

        double mul[12];
        for (int c = 0; c < 12; ++c)
        {
            mul[c] = fastExp2 ((vibCents + driftCents + age * detuneCents[c]) / 1200.0);
            phase0[c] += baseInc[c] * mul[c];
            if (phase0[c] >= 1.0) phase0[c] -= 1.0;
        }

        float bus[kBuses] {};
        if (na > 0)
        {
            for (int a = 0; a < na; ++a)
            {
                Key& k = keys[active[a]];
                if (k.down) k.env = std::min (1.0f, k.env + attack);
                else k.env = std::max (0.0f, k.env - release);
                if (k.env <= 0.0f) continue;
                for (int b = 0; b < kBuses; ++b)
                {
                    if (! busUsed[b]) continue;
                    int p = active[a] + kBusOffset[b];
                    while (p > 108) p -= 12;
                    while (p < 12) p += 12;
                    const int c = p % 12, oct = p / 12;
                    const double scale = (double) (1 << oct);
                    const double ph0 = phase0[c] * scale;
                    const double ph = ph0 - std::floor (ph0);
                    const double dt = baseInc[c] * mul[c] * scale;
                    double ph2 = ph + 0.5; if (ph2 >= 1.0) ph2 -= 1.0;
                    const float sq = (ph < 0.5 ? 1.0f : -1.0f) + polyBlep (ph, dt) - polyBlep (ph2, dt);
                    bus[b] += k.env * sq;
                }
            }
        }

        float y = 0.0f, strings = 0.0f;
        if (busUsed[0])
        {
            f[0].tick (bus[0]); f[1].tick (f[0].lp);
            y += tg[0] * 1.8f * f[1].lp;
            f[2].tick (bus[0]);
            strings += tg[1] * 0.8f * f[2].hp;
        }
        if (busUsed[1])
        {
            f[3].tick (bus[1]); f[4].tick (f[3].lp);
            y += tg[2] * 1.5f * f[4].lp;
            f[5].tick (bus[1]);
            y += tg[3] * 0.8f * f[5].bp;
            f[6].tick (bus[1]);
            y += tg[4] * (1.1f * f[6].bp + 0.22f * bus[1]);
            f[7].tick (bus[1]);
            strings += tg[5] * 1.0f * f[7].hp;
        }
        if (busUsed[2])
        {
            f[8].tick (bus[2]); f[9].tick (f[8].lp);
            y += tg[6] * 1.2f * f[9].lp;
            f[10].tick (bus[2]);
            strings += tg[7] * 0.9f * f[10].hp;
        }
        if (busUsed[3]) { f[11].tick (bus[3]); y += tg[8] * 0.9f * f[11].lp; }
        if (busUsed[4]) { f[12].tick (bus[4]); y += tg[9] * 0.9f * f[12].lp; }
        f[13].tick (strings);
        y += f[13].lp;

        out[s] = highShelf.tick (lowShelf.tick (y * 0.12f));
    }
}
}
