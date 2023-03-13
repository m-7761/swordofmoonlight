#pragma once

//DUPLICATE nanovg.c

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>

#include "nanovg/nanovg.h"
#define FONTSTASH_IMPLEMENTATION
#include "nanovg/fontstash.h"

#ifndef NVG_NO_STB
#define STB_IMAGE_IMPLEMENTATION
#include "nanovg/stb_image.h"
#endif 