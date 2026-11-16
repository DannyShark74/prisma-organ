#include "Engine.h"

namespace prisma
{
void Engine::prepare (double sampleRate, int block)
{
    sr = sampleRate;
    maxBlock = std::max (32, block);
    tw.prepare (sr, maxBlock);
    cb.prepare (sr, maxBlock);
    wahA.prepare (sr); wahB.prepare (sr);
    driveA.prepare (sr, maxBlock); driveB.prepare (sr, maxBlock);
    rotary.prepare (sr, maxBlock);
    echo.prepare (sr, maxBlock);
    room.prepare (sr, maxBlock);
    for (auto* v : { &bufTw, &bufCb, &busA, &busB }) v->assign ((size_t) maxBlock, 0.0f);
    const int sm = (int) (sr * 0.02);
    twGain.setLength (sm); cbGain.setLength (sm); master.setLength (sm); expression.setLength ((int) (sr * 0.01));
    setParams (params);
    twGain.snap (twGain.target); cbGain.snap (cbGain.target); master.snap (master.target);
    expression.snap (exprTarget);
}

void Engine::reset()
{
    tw.reset(); cb.reset();
    wahA.reset(); wahB.reset(); driveA.reset(); driveB.reset();
    rotary.reset(); echo.reset(); room.reset();
    std::fill (std::begin (sounding), std::end (sounding), false);
    std::fill (std::begin (sustained), std::end (sustained), false);
    sustain = false;
}

void Engine::setParams (const EngineParams& p)
{
    params = p;
    if (p.rotary.speed != lastUiSpeed) { lastUiSpeed = p.rotary.speed; speedOverride = -1; }
    tw.setSettings (p.tw);
    cb.setSettings (p.cb);
    wahA.setSettings (p.wah); wahB.setSettings (p.wah);
    driveA.setSettings (p.drive); driveB.setSettings (p.drive);
    auto rs = p.rotary;
    if (speedOverride >= 0) rs.speed = speedOverride;
    rotary.setSettings (rs);
    echo.setSettings (p.echo);
    room.setSettings (p.room);
    twGain.set (std::clamp (p.twLevel, 0.0f, 1.0f));
    cbGain.set (std::clamp (p.cbLevel, 0.0f, 1.0f));
    master.set (p.masterDb <= -59.9f ? 0.0f : dbToGain (p.masterDb));
}

void Engine::handle (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        if (sounding[n]) { tw.noteOff (noteShift[n]); cb.noteOff (noteShift[n]); }
        noteShift[n] = std::clamp (n + 12 * params.octave, 0, 127);
        tw.noteOn (noteShift[n]);
        cb.noteOn (noteShift[n]);
        sounding[n] = true;
        sustained[n] = false;
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();
        if (! sounding[n]) return;
        if (sustain) { sustained[n] = true; return; }
        tw.noteOff (noteShift[n]); cb.noteOff (noteShift[n]);
        sounding[n] = false;
    }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1)
        {
            speedOverride = v >= 64 ? 2 : 1;
            auto rs = params.rotary; rs.speed = speedOverride; rotary.setSettings (rs);
        }
        else if (cc == 11)
            exprTarget = std::pow ((float) v / 127.0f, 1.8f);
        else if (cc == 64)
        {
            sustain = v >= 64;
            if (! sustain)
                for (int n = 0; n < 128; ++n)
                    if (sustained[n])
                    {
                        sustained[n] = false; sounding[n] = false;
                        tw.noteOff (noteShift[n]); cb.noteOff (noteShift[n]);
                    }
        }
        else if (cc == 120 || cc == 123)
        {
            tw.allNotesOff(); cb.allNotesOff();
            std::fill (std::begin (sounding), std::end (sounding), false);
            std::fill (std::begin (sustained), std::end (sustained), false);
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        tw.allNotesOff(); cb.allNotesOff();
        std::fill (std::begin (sounding), std::end (sounding), false);
    }
}

void Engine::process (float* L, float* R, int n, const juce::MidiBuffer& midi)
{
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = std::clamp (meta.samplePosition, 0, n);
        while (pos < at)
        {
            const int len = std::min (maxBlock, at - pos);
            renderChunk (L + pos, R + pos, len);
            pos += len;
        }
        handle (meta.getMessage());
    }
    while (pos < n)
    {
        const int len = std::min (maxBlock, n - pos);
        renderChunk (L + pos, R + pos, len);
        pos += len;
    }
}

void Engine::renderChunk (float* L, float* R, int n)
{
    float* t = bufTw.data();
    float* c = bufCb.data();
    float* a = busA.data();
    float* b = busB.data();

    const bool twLive = twGain.target > 0.0f || twGain.cur > 0.0f;
    const bool cbLive = cbGain.target > 0.0f || cbGain.cur > 0.0f;
    if (twLive) tw.render (t, n); else std::fill (t, t + n, 0.0f);
    if (cbLive) cb.render (c, n); else std::fill (c, c + n, 0.0f);

    const bool cbRot = params.comboThroughRotary;
    expression.set (exprTarget);
    for (int i = 0; i < n; ++i)
    {
        const float e = expression.next();
        const float tv = twGain.next() * t[i];
        const float cv = cbGain.next() * c[i];
        a[i] = e * (tv + (cbRot ? cv : 0.0f));
        b[i] = e * (cbRot ? 0.0f : cv);
    }

    wahA.process (a, n);
    driveA.process (a, n);
    rotary.process (a, L, R, n);

    if (cbLive && ! cbRot)
    {
        wahB.process (b, n);
        driveB.process (b, n);
        for (int i = 0; i < n; ++i) { L[i] += b[i]; R[i] += b[i]; }
    }

    echo.process (L, R, n);
    room.process (L, R, n);

    for (int i = 0; i < n; ++i)
    {
        const float g = master.next();
        L[i] *= g; R[i] *= g;
    }
}
}
