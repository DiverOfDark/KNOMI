#pragma once

#if defined(KNOMIV1)
#include "pinout_knomi_v1.h"
#elif defined(KNOMIV2)
#include "pinout_knomi_v2.h"
#else
#error "Unknown board"
#endif

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

#define CENTER_X (DISPLAY_WIDTH / 2)
#define CENTER_Y (DISPLAY_HEIGHT / 2)
