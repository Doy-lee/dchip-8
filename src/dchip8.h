#ifndef DCHIP8_H
#define DCHIP8_H

#include "dchip8_platform.h"

bool dchip8_load_rom(wchar_t *filePath);

void dchip8_update(PlatformRenderBuffer renderBuffer, PlatformInput input,
                   PlatformMemory memory, u32 cyclesToEmulate);

#endif
