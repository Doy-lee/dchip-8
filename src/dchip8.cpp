#include "common.h"
#include "dchip8_platform.h"

void dchip8_update(PlatformRenderBuffer *renderBuffer)
{
	ASSERT(renderBuffer.bytesPerPixel == 4);

	const i32 numPixels = renderBuffer->width * renderBuffer->height;
	u32 *bitmapBuffer   = (u32 *)renderBuffer->memory;
	for (i32 i = 0; i < numPixels; i++)
	{
		// NOTE: Win32 AlphaBlend requires the RGB components to be
		// premultiplied with alpha.
		f32 normA = 1.0f;
		f32 normR = (normA * 0.0f);
		f32 normG = (normA * 0.0f);
		f32 normB = (normA * 1.0f);

		u8 r = (u8)(normR * 255.0f);
		u8 g = (u8)(normG * 255.0f);
		u8 b = (u8)(normB * 255.0f);
		u8 a = (u8)(normA * 255.0f);

		u32 color       = (a << 24) | (r << 16) | (g << 8) | (b << 0);
		bitmapBuffer[i] = color;
	}
}
