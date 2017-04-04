#ifndef DCHIP_8_PLATFORM
#define DCHIP_8_PLATFORM

#include "dqnt.h"

typedef struct PlatformRenderBuffer
{
	i32   width;
	i32   height;
	i32   bytesPerPixel;
	void *memory;
} PlatformRenderBuffer;

enum Key
{
	key_up,
	key_down,
	key_left,
	key_right,
	key_escape,
	key_count,
};

typedef struct KeyState
{
	bool isDown;
	u32 transitionCount;
} KeyState;

typedef struct PlatformInput
{
	f32 deltaForFrame;

	union {
		KeyState key[key_count];
		struct
		{
			KeyState up;
			KeyState down;
			KeyState left;
			KeyState right;
			KeyState escape;
		};
	};
} PlatformInput;

typedef struct PlatformMemory
{
	void *permanentMem;
	u32   permanentMemSize;

	void *transientMem;
	u32   transientMemSize;
} PlatformMemory;

#endif
