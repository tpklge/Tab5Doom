#pragma once
#include <stdint.h>

// Called by the engine task only. Keep real and paused game time separate.
class ChexTicClock
{
    uint64_t origin = 0, paused = 0, freeze_start = 0, sample = 0;
    bool frozen = false, sampled = false;
public:
    void reset(uint64_t now)
    {
        origin = now;
        paused = freeze_start = sample = 0;
        frozen = sampled = false;
    }
    uint64_t realtime(uint64_t now) const { return now - origin; }
    uint64_t elapsed(uint64_t now) const
    {
        return (frozen ? freeze_start : now) - origin - paused;
    }
    int ticks(uint64_t now, bool save)
    {
        if (save && !frozen) { sample = now; sampled = true; }
        return static_cast<int>(elapsed(now) * 35 / 1000000);
    }
    void freeze(uint64_t now, bool value)
    {
        if (value == frozen) return;
        if (value) freeze_start = now;
        else {
            const uint64_t duration = now - freeze_start;
            paused += duration;
            if (sampled) sample += duration;
        }
        frozen = value;
    }
    bool is_frozen() const { return frozen; }
    int fraction(uint64_t now, uint32_t *deadline) const
    {
        if (deadline) *deadline = static_cast<uint32_t>((sample + 1000000 / 35) / 1000);
        if (!sampled) return 65536;
        uint64_t current = frozen ? freeze_start : now;
        uint64_t difference = current > sample ? current - sample : 0;
        if (difference >= 1000000 / 35) return 65536;
        return static_cast<int>(difference * 65536 * 35 / 1000000);
    }
};
