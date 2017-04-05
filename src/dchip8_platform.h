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

	key_7,
	key_8,
	key_9,
	key_0,

	key_U,
	key_I,
	key_O,
	key_P,

	key_J,
	key_K,
	key_L,
	key_colon,

	key_M,
	key_comma,
	key_dot,
	key_forward_slash,

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

			KeyState key_7;
			KeyState key_8;
			KeyState key_9;
			KeyState key_0;

			KeyState key_U;
			KeyState key_I;
			KeyState key_O;
			KeyState key_P;

			KeyState key_J;
			KeyState key_K;
			KeyState key_L;
			KeyState key_colon;

			KeyState key_M;
			KeyState key_comma;
			KeyState key_dot;
			KeyState key_forward_slash;
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
