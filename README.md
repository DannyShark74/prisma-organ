# Prisma

A psychedelic organ instrument (VST3 / AU, macOS universal) for personal use: the two organs of
late-60s / 70s space rock in one plugin, with the effects they were played through.
Everything is modelled, no samples.

## Signal flow

```
Tonewheel engine ─┐
                  ├─ expression (CC11) ─ wah ─ tube preamp ─ rotary speaker ─┐
Combo engine ─────┤ (when "Into rotary" is on)                                ├─ drum echo ─ room ─ volume
                  └─ expression ─ wah ─ tube preamp ─────────────────────────┘
```

| Part | How it works |
|---|---|
| Tonewheel | 91 wheels that never stop spinning, tuned by the real gear ratios (20 rev/s shaft, 2 to 192 teeth; the top 7 wheels run a fifth gear and are ~2 cents sharp). 9 busbars (16' to 1') with fold-back, 3 dB per drawbar step. Each key has 9 contacts that close at slightly different times, so the click comes from the contacts themselves, plus a short burst of contact noise. Leakage and crosstalk between wheels. Single-trigger percussion (2nd/3rd, fast/slow, soft/normal) that takes over the 1' busbar. Vibrato / chorus V1–V3 / C1–C3 from a scanned delay line at 6.86 Hz. "Deep 16'" turns off the lowest-octave fold-back. |
| Combo | 12 master oscillators, every octave derived by flip-flop division, so all octaves of a note are phase locked (band-limited square waves). Keys switch the squares onto 5 footage busbars; fixed formant filters make the tab voices (Bass 16', Strings 16', Flute 8', Oboe 8', Trumpet 8', Strings 8', Flute 4', Strings 4', 2 2/3', 2'). Pitch vibrato light/heavy, slow/fast. "Age" mistunes the master oscillators slightly and lets them drift. |
| Wah | Resonant band-pass 350 Hz – 2.2 kHz: pedal position, LFO, or envelope. |
| Drive | Asymmetric triode curve at 2x oversampling, bias that sags with the signal (transformer "pumping"), tone filter. |
| Rotary | 800 Hz crossover (1 kHz for the small cabinet). Horn and drum turn at their own speeds with their own inertia (slow 0.83 / 0.67 Hz, fast 6.75 / 5.83 Hz). Each of two virtual mics hears the horn mouth and its back reflection: Doppler from the changing distance, level and brightness from the horn's direction. The drum is mostly amplitude movement. Mod wheel switches slow / fast. |
| Drum echo | Magnetic drum with 4 playback heads at 1x–4x the head spacing (40–160 ms). Echo = single repeats, Repeat = feedback from the selected heads, Swell = feedback from all four heads (the regenerating wash). Drum eccentricity wow and felt flutter, saturating record path, tone, wear, heads 1/3 left and 2/4 right. |
| Room | 8-line feedback delay network. |

MIDI: CC1 rotary speed, CC11 expression, CC64 sustain.

## Building

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPRISMA_BUILD_TOOLS=ON
cmake --build build --target Prisma_VST3 PrismaRender
build/PrismaRender_artefacts/Release/PrismaRender --test
build/PrismaRender_artefacts/Release/PrismaRender --demos demos
```

`PrismaRender --test` checks the tuning, the played pitch of both engines, silence when idle,
every factory preset for finite output and a sane level, CPU, and 44.1 / 96 kHz.

CI (GitHub Actions, macOS) builds a universal VST3 + AU, runs the engine checks, builds and tests
the `.pkg` installer, validates with pluginval and auval and publishes the installer to the
`downloads` branch.
