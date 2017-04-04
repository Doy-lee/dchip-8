#ifndef DCHIP_8_PLATFORM
#define DCHIP_8_PLATFORM

#include "Common.h"

typedef struct PlatformRenderBuffer
{
	i32   width;
	i32   height;
	i32   bytesPerPixel;
	void *memory;
} PlatformRenderBuffer;

#endif
