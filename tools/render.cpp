// Offline checks and demo renders for the Prisma engine.
//   PrismaRender --test                      engine checks (pitch, levels, NaN, CPU)
//   PrismaRender --demos <outdir> [sr]       one demo clip per factory preset
//   PrismaRender --preset "<name>" <out.wav> [sr]
#include <juce_audio_formats/juce_audio_formats.h>
#include "Engine.h"
#include "ParamTable.h"
#include <chrono>
#include <cstdio>

using namespace prisma;

static EngineParams paramsFor (const Preset* preset, std::map<std::string, float> overrides = {})
{
    std::map<std::string, float> v;
    for (auto& d : paramTable()) v[d.id] = d.def;
    if (preset) for (auto& kv : preset->values) v[kv.first] = kv.second;
    for (auto& kv : overrides) v[kv.first] = kv.second;
    return engineParamsFrom ([&v] (const std::string& id)
    {
        auto it = v.find (id);
        if (it == v.end()) { std::fprintf (stderr, "unknown param %s\n", id.c_str()); std::exit (2); }
        return it->second;
    });
}

struct Event { double t; juce::MidiMessage m; };

static std::vector<Event> demoSequence (bool melodic)
{
    std::vector<Event> ev;
    const std::vector<std::vector<int>> chords = { { 45, 55, 60, 64, 71 }, { 41, 57, 60, 64, 69 },
                                                   { 38, 53, 57, 60, 64 }, { 40, 56, 62, 67 } };
    double t = 0.1;
    for (size_t c = 0; c < chords.size(); ++c)
    {
        for (int n : chords[c]) ev.push_back ({ t, juce::MidiMessage::noteOn (1, n, (juce::uint8) 100) });
        if (c == 3) ev.push_back ({ t + 0.5, juce::MidiMessage::controllerEvent (1, 1, 127) });   // rotor to fast
        t += 2.0;
        for (int n : chords[c]) ev.push_back ({ t - 0.02, juce::MidiMessage::noteOff (1, n) });
    }
    ev.push_back ({ t, juce::MidiMessage::controllerEvent (1, 1, 0) });
    const std::vector<int> line = melodic ? std::vector<int> { 76, 74, 72, 69, 67, 69 } : std::vector<int> { 64, 67, 69 };
    for (int n : line)
    {
        ev.push_back ({ t, juce::MidiMessage::noteOn (1, n, (juce::uint8) 100) });
        t += melodic ? 0.42 : 0.6;
        ev.push_back ({ t - 0.05, juce::MidiMessage::noteOff (1, n) });
    }
    std::stable_sort (ev.begin(), ev.end(), [] (const Event& a, const Event& b) { return a.t < b.t; });
    return ev;
}

static juce::AudioBuffer<float> render (const EngineParams& p, const std::vector<Event>& ev, double seconds, double sr,
                                        double* cpuSeconds = nullptr, int block = 256)
{
    Engine e;
    e.prepare (sr, block);
    e.setParams (p);
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> out (2, total);
    out.clear();
    size_t next = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        juce::MidiBuffer mb;
        while (next < ev.size() && (int) (ev[next].t * sr) < pos + n)
        {
            mb.addEvent (ev[next].m, std::max (0, (int) (ev[next].t * sr) - pos));
            ++next;
        }
        e.process (out.getWritePointer (0, pos), out.getWritePointer (1, pos), n, mb);
    }
    if (cpuSeconds) *cpuSeconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    return out;
}

static bool writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
{
    f.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
    if (! os) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (! w) return false;
    os.release();
    return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

static double zeroCrossFreq (const float* x, int n, double sr)
{
    int first = -1, last = -1, count = 0;
    for (int i = 1; i < n; ++i)
        if (x[i - 1] < 0 && x[i] >= 0)
        {
            if (first < 0) first = i; else { last = i; ++count; }
        }
    if (count < 2) return 0;
    // refine with linear interpolation at both ends
    auto frac = [x] (int i) { return (double) i - 1 + (double) (-x[i - 1]) / (double) (x[i] - x[i - 1]); };
    return count * sr / (frac (last) - frac (first));
}

static int runTests()
{
    int failures = 0;
    auto check = [&failures] (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++failures; };
    const double sr = 48000;

    // wheel frequencies against equal temperament (wheel 13 = C2 = MIDI 36)
    double worst = 0;
    for (int w = 1; w <= Tonewheel::kWheels; ++w)
    {
        const double et = 440.0 * std::pow (2.0, (w + 23 - 69) / 12.0);
        worst = std::max (worst, std::abs (1200.0 * std::log2 (Tonewheel::wheelFrequency (w) / et)));
    }
    std::printf ("      worst tonewheel deviation from equal temperament: %.2f cents\n", worst);
    check (worst < 2.5, "tonewheel gear ratios within 2.5 cents of equal temperament (top 7 wheels run a fifth gear, ~2 cents sharp, as on the real generator)");
    check (std::abs (Tonewheel::wheelFrequency (46) - 440.0) < 0.01, "wheel 46 = 440 Hz");

    // played pitch, tonewheel 8' only, everything dry
    std::map<std::string, float> dry { { "rot_on", 0 }, { "vib_mode", 0 }, { "drive", 0 }, { "room_mix", 0 }, { "leak", 0 },
                                       { "click", 0 }, { "db1", 0 }, { "db2", 0 }, { "db3", 8 } };
    {
        std::vector<Event> ev { { 0.0, juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100) } };
        auto b = render (paramsFor (nullptr, dry), ev, 1.5, sr);
        const double f = zeroCrossFreq (b.getReadPointer (0) + (int) (0.3 * sr), (int) sr, sr);
        std::printf ("      tonewheel A4 measured %.3f Hz\n", f);
        check (std::abs (f - 440.0) < 0.3, "tonewheel A4 plays 440 Hz");
    }
    {
        auto c = dry; c["tw_level"] = 0; c["cb_level"] = 1; c["cb_age"] = 0; c["cb_vib"] = 0;
        c["cb_str8"] = 0; c["cb_flute8"] = 1;
        std::vector<Event> ev { { 0.0, juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100) } };
        auto b = render (paramsFor (nullptr, c), ev, 1.5, sr);
        const double f = zeroCrossFreq (b.getReadPointer (0) + (int) (0.3 * sr), (int) sr, sr);
        std::printf ("      combo A4 measured %.3f Hz\n", f);
        check (std::abs (f - 440.0) < 0.3, "combo A4 plays 440 Hz");
    }

    // silence in -> silence out (no denormal blow-ups, no hiss when idle)
    {
        auto b = render (paramsFor (nullptr), {}, 2.0, sr);
        check (b.getMagnitude (0, b.getNumSamples()) < 1.0e-4f, "idle output is silent");
    }

    // every preset: finite, sane level, CPU
    double worstCpu = 0;
    for (auto& pr : factoryPresets())
    {
        double cpu = 0;
        const auto ev = demoSequence (true);
        auto b = render (paramsFor (&pr), ev, 14.0, sr, &cpu);
        bool finite = true;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (c, i))) finite = false;
        const float peak = b.getMagnitude (0, b.getNumSamples());
        const float rms = 0.5f * (b.getRMSLevel (0, 0, b.getNumSamples()) + b.getRMSLevel (1, 0, b.getNumSamples()));
        const double rt = cpu / 14.0;
        worstCpu = std::max (worstCpu, rt);
        std::printf ("      %-20s peak %6.1f dB  rms %6.1f dB  cpu %5.2f%%%s\n", pr.name.c_str(),
                     juce::Decibels::gainToDecibels (peak), juce::Decibels::gainToDecibels (rms), rt * 100.0, finite ? "" : "  NaN!");
        check (finite, (pr.name + " finite").c_str());
        check (peak < 1.5f && peak > 0.02f, (pr.name + " level in range").c_str());
    }
    std::printf ("      worst CPU (one core): %.2f%% of real time\n", worstCpu * 100.0);
    check (worstCpu < 0.25, "CPU below 25% of one core");

    // sample-rate independence: 44.1k and 96k render without trouble
    for (double rate : { 44100.0, 96000.0 })
    {
        auto b = render (paramsFor (&factoryPresets()[0]), demoSequence (false), 6.0, rate);
        check (b.getMagnitude (0, b.getNumSamples()) < 1.5f, rate > 50000 ? "96 kHz render ok" : "44.1 kHz render ok");
    }

    std::printf ("%s (%d failures)\n", failures == 0 ? "ALL OK" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}

int main (int argc, char** argv)
{
    if (argc >= 2 && juce::String (argv[1]) == "--test") return runTests();
    if (argc >= 3 && juce::String (argv[1]) == "--demos")
    {
        const double sr = argc >= 4 ? juce::String (argv[3]).getDoubleValue() : 48000.0;
        juce::File dir (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        dir.createDirectory();
        int i = 0;
        for (auto& pr : factoryPresets())
        {
            const bool melodic = pr.category == "Psych 60s" || pr.category == "Ambient";
            auto b = render (paramsFor (&pr), demoSequence (melodic), 14.0, sr);
            auto name = juce::String (++i).paddedLeft ('0', 2) + " " + juce::String (pr.name) + ".wav";
            writeWav (b, sr, dir.getChildFile (name));
            std::printf ("wrote %s\n", name.toRawUTF8());
        }
        return 0;
    }
    if (argc >= 4 && juce::String (argv[1]) == "--preset")
    {
        const double sr = argc >= 5 ? juce::String (argv[4]).getDoubleValue() : 48000.0;
        for (auto& pr : factoryPresets())
            if (pr.name == argv[2])
            {
                const bool melodic = pr.category == "Psych 60s" || pr.category == "Ambient";
                auto b = render (paramsFor (&pr), demoSequence (melodic), 14.0, sr);
                return writeWav (b, sr, juce::File::getCurrentWorkingDirectory().getChildFile (argv[3])) ? 0 : 1;
            }
        std::fprintf (stderr, "no preset %s\n", argv[2]);
        return 1;
    }
    std::fprintf (stderr, "usage: PrismaRender --test | --demos <dir> [sr] | --preset <name> <out.wav> [sr]\n");
    return 2;
}
