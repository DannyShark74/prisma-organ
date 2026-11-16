#include "Effects.h"
#include <juce_dsp/juce_dsp.h>

namespace prisma
{
// =================================================================== Wah
void Wah::prepare (double sampleRate)
{
    sr = sampleRate;
    envAtt = (float) (1.0 - std::exp (-1.0 / (0.004 * sr)));
    envRel = (float) (1.0 - std::exp (-1.0 / (0.12 * sr)));
    posCoef = (float) (1.0 - std::exp (-1.0 / (0.008 * sr)));
    onAmt.setLength ((int) (sr * 0.02));
    onAmt.snap (set.on ? 1.0f : 0.0f);
    reset();
}

void Wah::reset() { svf.reset(); env = 0; lfo = 0; }

void Wah::process (float* x, int n)
{
    if (! set.on && onAmt.cur <= 0.0f && onAmt.left == 0) return;
    const float res = std::clamp (set.res, 0.0f, 1.0f);
    const double q = 1.5 + res * 7.0;
    const double lfoInc = std::clamp ((double) set.rate, 0.02, 12.0) / sr;
    for (int i = 0; i < n; ++i)
    {
        const float in = x[i];
        const float a = std::abs (in);
        env += (a - env) * (a > env ? envAtt : envRel);
        float target = set.pos;
        if (set.mode == 1)
        {
            lfo += lfoInc; if (lfo >= 1.0) lfo -= 1.0;
            target = 0.5f + 0.5f * SineTable::get() (lfo);
            target = set.pos * 0.3f + target * 0.7f;
        }
        else if (set.mode == 2)
            target = std::clamp (set.pos * 0.6f + env * 2.5f, 0.0f, 1.0f);
        posSm += (target - posSm) * posCoef;
        if ((counter++ & 7) == 0)
            svf.set (sr, 350.0 * std::pow (2200.0 / 350.0, (double) posSm), q);
        svf.tick (in);
        const float wet = svf.bp * (1.7f / (float) std::sqrt (q)) + 0.12f * svf.lp;
        const float o = onAmt.next();
        x[i] = in + o * (wet - in);
    }
}

// =================================================================== Drive
Drive::Drive() = default;
Drive::~Drive() = default;

void Drive::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    os = std::make_unique<juce::dsp::Oversampling<float>> (1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    os->initProcessing ((size_t) maxBlock);
    gain.setLength ((int) (sr * 0.03));
    makeup.setLength ((int) (sr * 0.03));
    envCoef = (float) (1.0 - std::exp (-1.0 / (0.05 * sr * 2.0)));
    pre.highpass (sr * 2.0, 35.0, 0.7);
    dcBlock.set (sr, 12.0);
    setSettings ({});
    gain.snap (gain.target); makeup.snap (makeup.target);
    reset();
}

void Drive::reset()
{
    if (os) os->reset();
    env = 0; pre.reset(); post.reset(); dcBlock.reset();
}

void Drive::setSettings (const DriveSettings& s)
{
    const float d = std::clamp (s.drive, 0.0f, 1.0f);
    const float g = dbToGain (d * 34.0f);
    gain.set (g);
    makeup.set (0.32f / std::tanh (g * 0.32f));
    const double fc = 2500.0 * std::pow (12000.0 / 2500.0, (double) std::clamp (s.tone, 0.0f, 1.0f)) * (1.0 - 0.3 * d);
    post.lowpass (sr * 2.0, std::min (fc, sr * 0.9), 0.65);
}

void Drive::process (float* x, int n)
{
    float* chans[1] = { x };
    juce::dsp::AudioBlock<float> block (chans, 1, (size_t) n);
    auto up = os->processSamplesUp (block);
    float* u = up.getChannelPointer (0);
    const int m = (int) up.getNumSamples();
    for (int i = 0; i < m; ++i)
    {
        if ((i & 1) == 0) { gain.next(); makeup.next(); }
        const float v = pre.tick (u[i]) * gain.cur;
        env += (std::abs (v) - env) * envCoef;
        // asymmetric triode-like curve; the bias sags with the signal ("pumping" through the transformer)
        const float bias = 0.18f - 0.12f * std::min (1.0f, env * 0.5f);
        const float y = std::tanh (v + bias) - std::tanh (bias);
        u[i] = post.tick (y * makeup.cur);
    }
    os->processSamplesDown (block);
    for (int i = 0; i < n; ++i) x[i] = dcBlock.hp (x[i]);
}

// =================================================================== Rotary
namespace
{
    constexpr float kSoundSpeed = 343.0f;
    struct RotorSpec { float hornSlow, hornFast, drumSlow, drumFast; };
    constexpr RotorSpec kSpec[2] = { { 0.83f, 6.75f, 0.67f, 5.83f },     // large cabinet
                                     { 0.90f, 7.05f, 0.75f, 6.20f } };   // small cabinet
}

void Rotary::prepare (double sampleRate, int)
{
    sr = sampleRate;
    horn.prepare ((int) (sr * 0.006) + 8);
    drum.prepare ((int) (sr * 0.006) + 8);
    onAmt.setLength ((int) (sr * 0.03));
    onAmt.snap (set.on ? 1.0f : 0.0f);
    cabBuilt = -1;
    updateCab();
    reset();
}

void Rotary::reset()
{
    horn.reset(); drum.reset();
    xlp1.reset(); xlp2.reset(); xhp1.reset(); xhp2.reset();
    for (int c = 0; c < 2; ++c) { eqLo[c].reset(); eqHi[c].reset(); eqTop[c].reset(); }
    for (auto& v : hz) v = 0;
}

void Rotary::updateCab()
{
    if (cabBuilt == set.cab) return;
    cabBuilt = set.cab;
    const bool small = set.cab == 1;
    const double xo = small ? 1000.0 : 800.0;
    xlp1.lowpass (sr, xo, 0.7071); xlp2.lowpass (sr, xo, 0.7071);
    xhp1.highpass (sr, xo, 0.7071); xhp2.highpass (sr, xo, 0.7071);
    for (int c = 0; c < 2; ++c)
    {
        eqLo[c].peak (sr, small ? 180.0 : 100.0, small ? 1.2 : 1.0, 3.0);
        eqHi[c].peak (sr, small ? 2200.0 : 2600.0, 1.0, 2.0);
        eqTop[c].lowpass (sr, std::min (small ? 6500.0 : 8500.0, sr * 0.45), 0.7);
    }
    hornR = small ? 0.13f : 0.16f;
    drumR = small ? 0.09f : 0.12f;
    hornLpLo = (float) (1.0 - std::exp (-kTwoPi * 2400.0 / sr));
    hornLpHi = (float) (1.0 - std::exp (-kTwoPi * std::min (14000.0, sr * 0.45) / sr));
}

void Rotary::setSettings (const RotarySettings& s)
{
    set = s;
    onAmt.set (s.on ? 1.0f : 0.0f);
    updateCab();
}

void Rotary::process (const float* in, float* outL, float* outR, int n)
{
    const auto& sine = SineTable::get();
    const auto& sp = kSpec[std::clamp (set.cab, 0, 1)];
    const float hornTarget = set.speed == 2 ? sp.hornFast : (set.speed == 1 ? sp.hornSlow : 0.0f);
    const float drumTarget = set.speed == 2 ? sp.drumFast : (set.speed == 1 ? sp.drumSlow : 0.0f);
    const float inertia = 0.35f * std::pow (2.0f / 0.35f, std::clamp (set.inertia, 0.0f, 1.0f));
    auto coef = [this] (float tau) { return (float) (1.0 - std::exp (-1.0 / (std::max (0.01f, tau) * sr))); };
    const float hornC = coef ((hornTarget > hornSpeed ? 0.5f : 0.9f) * inertia);
    const float drumC = coef ((drumTarget > drumSpeed ? 2.5f : 3.2f) * inertia);

    const float spread = std::clamp (set.width, 0.0f, 1.0f) * 0.5f;   // in turns: 0.5 = mics 180 deg apart
    const float micAng[2] = { -spread * 0.5f, spread * 0.5f };
    const float drumMicAng[2] = { -spread * 0.25f, spread * 0.25f };
    const float bal = std::clamp (set.balance, -1.0f, 1.0f);
    const float hornGain = std::min (1.0f, 1.0f + bal), drumGain = std::min (1.0f, 1.0f - bal);
    const float hornSpan = hornR / kSoundSpeed * (float) sr, drumSpan = drumR / kSoundSpeed * (float) sr;
    const float hornBase = 0.0004f * (float) sr, reflBase = 0.0016f * (float) sr, drumBase = 0.0003f * (float) sr;

    auto cosTurn = [&sine] (double t) { t += 0.25; t -= std::floor (t); return sine (t); };

    for (int i = 0; i < n; ++i)
    {
        hornSpeed += (hornTarget - hornSpeed) * hornC;
        drumSpeed += (drumTarget - drumSpeed) * drumC;
        hornAng += hornSpeed / sr; if (hornAng >= 1.0) hornAng -= 1.0;
        drumAng += drumSpeed / sr; if (drumAng >= 1.0) drumAng -= 1.0;

        const float x = in[i];
        const float lo = xlp2.tick (xlp1.tick (x));
        const float hi = xhp2.tick (xhp1.tick (x));
        horn.push (hi);
        drum.push (lo);

        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            // horn: the mouth swings towards and away from the mic -> doppler, level and brightness
            const float cv = cosTurn (hornAng - micAng[c]);
            const float q = 0.5f + 0.5f * cv, q2 = q * q;
            const float d1 = hornBase + hornSpan * (1.0f - cv);
            float& s1 = hz[c * 2];
            s1 += (hornLpLo + (hornLpHi - hornLpLo) * q2) * ((0.3f + 0.7f * q2) * horn.read (d1) - s1);
            // the back of the horn and the cabinet wall reflection, opposite phase of the rotation
            const float qr = 1.0f - q, qr2 = qr * qr;
            const float d2 = reflBase + hornSpan * (1.0f + cv);
            float& s2 = hz[c * 2 + 1];
            s2 += (hornLpLo + (hornLpHi - hornLpLo) * qr2) * (0.32f * (0.3f + 0.7f * qr2) * horn.read (d2) - s2);

            // drum: a rotating baffle, mostly amplitude movement with a little doppler
            const float dv = cosTurn (drumAng - drumMicAng[c]);
            const float dq = 0.5f + 0.5f * dv;
            const float dd = drumBase + drumSpan * (1.0f - dv);
            const float dr = drumBase + drumSpan * (1.0f + dv) + 0.0011f * (float) sr;
            const float dOut = (0.5f + 0.5f * dq) * drum.read (dd) + 0.3f * (1.0f - 0.5f * dq) * drum.read (dr);

            const float wet = 1.25f * (hornGain * (s1 + s2) + drumGain * dOut);
            out[c] = eqTop[c].tick (eqHi[c].tick (eqLo[c].tick (wet)));
        }
        const float o = onAmt.next();
        outL[i] = x + o * (out[0] - x);
        outR[i] = x + o * (out[1] - x);
    }
}

// =================================================================== Drum echo
void DrumEcho::prepare (double sampleRate, int)
{
    sr = sampleRate;
    line.prepare ((int) (sr * 0.8));
    spacing.setLength ((int) (sr * 0.12));
    onAmt.setLength ((int) (sr * 0.02));
    fb.setLength ((int) (sr * 0.02));
    mix.setLength ((int) (sr * 0.02));
    fbHp.set (sr, 140.0);
    setSettings (set);
    spacing.snap (spacing.target); onAmt.snap (onAmt.target); fb.snap (fb.target); mix.snap (mix.target);
    reset();
}

void DrumEcho::reset()
{
    line.reset(); fbLp.reset(); fbHp.reset(); outLpL.reset(); outLpR.reset();
    wow = 0; flutter = flutterTarget = 0;
}

void DrumEcho::setSettings (const EchoSettings& s)
{
    set = s;
    spacing.set (std::clamp (s.timeMs, 30.0f, 170.0f) * 0.001f * (float) sr);
    onAmt.set (s.on ? 1.0f : 0.0f);
    fb.set (std::clamp (s.feedback, 0.0f, 1.0f));
    mix.set (std::clamp (s.mix, 0.0f, 1.0f));
    const float t = std::clamp (s.tone, 0.0f, 1.0f), w = std::clamp (s.wear, 0.0f, 1.0f);
    fbLp.set (sr, 2200.0 * std::pow (3.4, (double) t) * (1.0 - 0.35 * w));
    const double outFc = 3000.0 * std::pow (3.3, (double) t) * (1.0 - 0.3 * w);
    outLpL.set (sr, outFc); outLpR.set (sr, outFc);
}

void DrumEcho::process (float* L, float* R, int n)
{
    if (! set.on && onAmt.cur <= 0.0f && onAmt.left == 0) return;
    const auto& sine = SineTable::get();
    const float w = std::clamp (set.wear, 0.0f, 1.0f);
    const float sp = std::clamp (set.spread, 0.0f, 1.0f);
    int nSel = 0;
    for (bool h : set.head) nSel += h ? 1 : 0;
    const float selNorm = nSel > 0 ? 1.0f / std::sqrt ((float) nSel) : 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float T = spacing.next();
        const float on = onAmt.next(), f = fb.next(), m = mix.next();
        const float in = 0.5f * (L[i] + R[i]);

        // drum eccentricity (once per revolution) plus random flutter from the felt pads
        wow += 1.0 / (5.0 * T); if (wow >= 1.0) wow -= 1.0;
        if (--flutterCount <= 0) { flutterTarget = rng.bi(); flutterCount = (int) (sr * 0.03); }
        flutter += (flutterTarget - flutter) * 0.0015f;
        const float mod = 1.0f + (0.0008f + 0.004f * w) * sine (wow) + (0.0004f + 0.002f * w) * flutter;

        float h[4], sum = 0, sel = 0, wl = 0, wr = 0;
        for (int k = 0; k < 4; ++k)
        {
            h[k] = line.read ((float) (k + 1) * T * mod + (float) k * 3.0f);
            sum += h[k];
            if (set.head[k])
            {
                sel += h[k];
                const bool left = (k % 2) == 0;
                const float pan = 0.5f + (left ? -0.5f : 0.5f) * sp;
                wl += h[k] * (1.0f - pan) * 1.4f;
                wr += h[k] * pan * 1.4f;
            }
        }
        wl *= selNorm; wr *= selNorm;

        float fbSig = 0.0f;
        if (set.mode == 1) fbSig = f * 0.95f * sel * selNorm;
        else if (set.mode == 2) fbSig = f * 0.42f * sum;
        fbSig = fbLp.lp (fbHp.hp (fbSig));

        float rec = in * on + fbSig + (rng.bi() * 0.0006f * w);
        rec = std::tanh (rec * 1.3f) * (1.0f / 1.3f);
        line.push (rec);

        L[i] += m * on * outLpL.lp (wl);
        R[i] += m * on * outLpR.lp (wr);
    }
}

// =================================================================== Room
void Room::prepare (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& l : lines) l.prepare ((int) (sr * 0.08));
    pre.prepare ((int) (sr * 0.02));
    for (auto& d : damp) d.set (sr, 6000.0);
    wetHpL.set (sr, 160.0); wetHpR.set (sr, 160.0);
    mix.setLength ((int) (sr * 0.02));
    builtSize = -1;
    setSettings (set);
    mix.snap (mix.target);
    reset();
}

void Room::reset()
{
    for (auto& l : lines) l.reset();
    for (auto& d : damp) d.reset();
    pre.reset(); wetHpL.reset(); wetHpR.reset();
}

void Room::setSettings (const RoomSettings& s)
{
    set = s;
    mix.set (std::clamp (s.mix, 0.0f, 1.0f));
    if (std::abs (s.size - builtSize) > 0.01f) build();
}

void Room::build()
{
    builtSize = set.size;
    static const float baseMs[N] = { 11.3f, 13.7f, 17.9f, 19.1f, 23.3f, 26.9f, 29.3f, 31.7f };
    const float sz = std::clamp (set.size, 0.0f, 1.0f);
    const float scale = 0.6f + 1.4f * sz;
    const float rt60 = 0.3f + 1.6f * sz;
    for (int i = 0; i < N; ++i)
    {
        len[i] = baseMs[i] * scale * 0.001f * (float) sr;
        g[i] = std::pow (10.0f, -3.0f * len[i] / (rt60 * (float) sr));
    }
}

void Room::process (float* L, float* R, int n)
{
    if (mix.target <= 0.0f && mix.cur <= 0.0f) return;
    const float preD = 0.008f * (float) sr;
    for (int i = 0; i < n; ++i)
    {
        const float m = mix.next();
        pre.push (0.5f * (L[i] + R[i]));
        const float in = pre.read (preD);
        float v[N];
        for (int k = 0; k < N; ++k) v[k] = damp[k].lp (lines[k].read (len[k]) * g[k]);
        // 8-point Hadamard
        for (int h = 1; h < N; h <<= 1)
            for (int a = 0; a < N; a += h * 2)
                for (int b = a; b < a + h; ++b) { const float x = v[b], y = v[b + h]; v[b] = x + y; v[b + h] = x - y; }
        const float norm = 0.35355339f;
        float wl = 0, wr = 0;
        for (int k = 0; k < N; ++k)
        {
            v[k] *= norm;
            lines[k].push (v[k] + ((k & 1) ? -in : in) * 0.5f);
            if (k & 1) wr += v[k]; else wl += v[k];
        }
        L[i] += m * 0.7f * wetHpL.hp (wl);
        R[i] += m * 0.7f * wetHpR.hp (wr);
    }
}
}
