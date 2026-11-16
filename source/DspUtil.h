#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace prisma
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }

// Deterministic small PRNG (xorshift), so every instance and every render sounds the same.
struct Rng
{
    uint32_t s = 0x9e3779b9u;
    explicit Rng (uint32_t seed = 0x9e3779b9u) : s (seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }   // [0,1)
    float bi() { return uni() * 2.0f - 1.0f; }                              // [-1,1)
};

// sin(2 pi x) for x in [0,1), table with linear interpolation
struct SineTable
{
    static constexpr int N = 4096;
    std::array<float, N + 1> t {};
    SineTable() { for (int i = 0; i <= N; ++i) t[(size_t) i] = (float) std::sin (kTwoPi * i / N); }
    inline float operator() (double phase) const
    {
        const double p = phase * N;
        const int i = (int) p;
        const float f = (float) (p - i);
        return t[(size_t) i] + f * (t[(size_t) i + 1] - t[(size_t) i]);
    }
    static const SineTable& get() { static SineTable s; return s; }
};

// Topology-preserving state variable filter (Zavalishin / Simper)
struct Svf
{
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    float ic1 = 0, ic2 = 0;
    float lp = 0, bp = 0, hp = 0;

    void set (double sr, double hz, double q)
    {
        hz = std::clamp (hz, 5.0, sr * 0.49);
        g = (float) std::tan (kPi * hz / sr);
        k = (float) (1.0 / std::max (0.05, q));
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void tick (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    void reset() { ic1 = ic2 = lp = bp = hp = 0; }
};

struct OnePole
{
    float a = 1, z = 0;
    void set (double sr, double hz) { a = (float) (1.0 - std::exp (-kTwoPi * std::min (hz, sr * 0.49) / sr)); }
    inline float lp (float x) { z += a * (x - z); return z; }
    inline float hp (float x) { return x - lp (x); }
    void reset() { z = 0; }
};

// RBJ biquad (used for fixed EQ shapes)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float tick (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void peak (double sr, double hz, double q, double db)
    {
        const double A = std::pow (10.0, db / 40.0), w = kTwoPi * hz / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double sr, double hz, double db)
    {
        const double A = std::pow (10.0, db / 40.0), w = kTwoPi * hz / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
              (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highShelf (double sr, double hz, double db)
    {
        const double A = std::pow (10.0, db / 40.0), w = kTwoPi * hz / sr, c = std::cos (w), s = std::sin (w);
        const double al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
              (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
    void lowpass (double sr, double hz, double q)
    {
        const double w = kTwoPi * hz / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        norm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double sr, double hz, double q)
    {
        const double w = kTwoPi * hz / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        norm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

private:
    void norm (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
};

// Fractional delay line with 4-point Hermite interpolation
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    void prepare (int maxSamples)
    {
        int n = 1;
        while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1; w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // d = delay in samples (>= 1): read the sample written d samples ago
    inline float read (float d) const
    {
        const float pos = (float) w - d;
        int i = (int) std::floor (pos);
        const float f = pos - (float) i;
        const float xm1 = buf[(size_t) ((i - 1) & mask)], x0 = buf[(size_t) (i & mask)];
        const float x1 = buf[(size_t) ((i + 1) & mask)], x2 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
};

// Linear smoother towards a target
struct Smoothed
{
    float cur = 0, target = 0, step = 0;
    int left = 0, len = 64;
    void setLength (int n) { len = std::max (1, n); }
    void snap (float v) { cur = target = v; left = 0; }
    void set (float v)
    {
        if (v == target) return;
        target = v; left = len; step = (target - cur) / (float) len;
    }
    inline float next()
    {
        if (left > 0) { cur += step; if (--left == 0) cur = target; }
        return cur;
    }
};
}
