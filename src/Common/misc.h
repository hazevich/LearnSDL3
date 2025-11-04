#pragma once

#include <cstdint>
#include "SDL3/SDL.h"

static const float NanosecondInSecond = 1e9f;

static inline float NanosecondsToSeconds(float nanoseconds)
{
    return nanoseconds / NanosecondInSecond;
}

static inline float SDL_GetTimeSeconds()
{
    return NanosecondsToSeconds(SDL_GetTicksNS());
}