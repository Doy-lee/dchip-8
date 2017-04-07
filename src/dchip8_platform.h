#ifndef DCHIP_8_PLATFORM
#define DCHIP_8_PLATFORM

#include "dqnt.h"

// NOTE: Platform buffers are expected to be top to bottom! I.e. origin is top
// left, also 4 bytes per pixel with packing order A R G B
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

	key_1,
	key_2,
	key_3,
	key_4,

	key_q,
	key_w,
	key_e,
	key_r,

	key_a,
	key_s,
	key_d,
	key_f,

	key_z,
	key_x,
	key_c,
	key_v,

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

			KeyState key_1;
			KeyState key_2;
			KeyState key_3;
			KeyState key_4;

			KeyState key_q;
			KeyState key_w;
			KeyState key_e;
			KeyState key_r;

			KeyState key_a;
			KeyState key_s;
			KeyState key_d;
			KeyState key_f;

			KeyState key_z;
			KeyState key_x;
			KeyState key_c;
			KeyState key_v;
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

typedef struct PlatformFile
{
	void *handle;
	u64   size;
} PlatformFile;

// Return true if successful, false if not
bool platform_open_file (const wchar_t *const file, PlatformFile *platformFile);
// Return the number of bytes read
u32  platform_read_file (PlatformFile file, void *buffer, u32 numBytesToRead);
void platform_close_file(PlatformFile *file);

#endif
