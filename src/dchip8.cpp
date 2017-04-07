#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "dchip8_platform.h"
#include "dqnt.h"
#include "stdio.h"

enum Chip8State
{
	chip8state_off,
	chip8state_await_input,
	chip8state_running,
};

typedef struct Chip8Controller
{
	bool key[0x10];
} Chip8Controller;

#define INIT_ADDRESS 0x200
typedef struct Chip8CPU
{
	union {
		u8 registerArray[16];
		struct
		{
			u8 V0;
			u8 V1;
			u8 V2;
			u8 V3;
			u8 V4;
			u8 V5;
			u8 V6;
			u8 V7;
			u8 V8;
			u8 V9;
			u8 VA;
			u8 VB;
			u8 VC;
			u8 VD;
			u8 VE;
			u8 VF;
		};
	};


	// NOTE: Timer that count at 60hz and when set above 0 will count down to 0.
	union {
		u8 dt;
		u8 delayTimer;
	};

	union {
		u8 st;
		u8 soundTimer;
	};

	// NOTE: Maximum value is 0xFFF, or 4095 or 12 bits
	union {
		u16 I;
		u16 indexRegister;
	};

	// NOTE: Maximum value is 0xFFF, or 4095 or 12 bits
	u16 programCounter;

	u8 stackPointer;
	u16 stack[16];

	// Metadata
	u8   storeKeyToRegisterIndex;
	f32  elapsedTime;
	enum Chip8State state;
} Chip8CPU;

FILE_SCOPE Chip8CPU     cpu;
FILE_SCOPE RandPCGState pcgState;

FILE_SCOPE void dchip8_init_memory(u8 *memory, u32 size)
{
	for (u32 i = 0; i < size; i++)
		memory[i] = 0;

	const u8 PRESET_FONTS[] =
	{
		// "0"
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0x90, // @--@ ----
		0x90, // @--@ ----
		0xF0, // @@@@----

		// "1"
		0x20, // --@- ----
		0x60, // -@@- ----
		0x20, // --@- ----
		0x20, // --@- ----
		0x70, // -@@@ ----

		// "2"
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----

		// "3"
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0xF0, // @@@@ ----

		// "4"
		0x90, // @--@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0x10, // ---@ ----

		// "5"
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0xF0, // @@@@ ----

		// "6"
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----

		// "7"
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0x20, // --@- ----
		0x40, // -@-- ----
		0x40, // -@-- ----

		// "8"
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----

		// "9"
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----
		0x10, // ---@ ----
		0xF0, // @@@@ ----

		// "A"
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0xF0, // @@@@ ----
		0x90, // @--@ ----
		0x90, // @--@ ----

		// "B"
		0xE0, // @@@- ----
		0x90, // @--@ ----
		0xE0, // @@@- ----
		0x90, // @--@ ----
		0xE0, // @@@- ----

		// "C"
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0x80, // @--- ----
		0x80, // @--- ----
		0xF0, // @@@@ ----

		// "D"
		0xE0, // @@@- ----
		0x90, // @--@ ----
		0x90, // @--@ ----
		0x90, // @--@ ----
		0xE0, // @@@- ----

		// "E"
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----

		// "F"
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0xF0, // @@@@ ----
		0x80, // @--- ----
		0x80  // @--- ----
	};

	for (i32 i = 0; i < DQNT_ARRAY_COUNT(PRESET_FONTS); i++)
		memory[i] = PRESET_FONTS[i];
}

FILE_SCOPE void dchip8_init_cpu(Chip8CPU *chip8CPU)
{
	memset(chip8CPU, 0, sizeof(*chip8CPU));

	// NOTE: Everything before 0x200 is reserved for the actual emulator
	chip8CPU->programCounter = INIT_ADDRESS;
	chip8CPU->I              = 0;
	chip8CPU->stackPointer   = 0;

	const u32 SEED = 0x8293A8DE;
	dqnt_rnd_pcg_seed(&pcgState, SEED);
}

FILE_SCOPE
void dchip8_debug_draw_half_colored_screen_internal(
    PlatformRenderBuffer renderBuffer)
{
	const i32 numPixels = renderBuffer.width * renderBuffer.height;
	u32 *bitmapBuffer   = (u32 *)renderBuffer.memory;
	for (i32 i = 0; i < numPixels; i++)
	{
		// NOTE: Win32 AlphaBlend requires the RGB components to be
		// premultiplied with alpha.
		if (i < numPixels * 0.5f)
		{
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
		else
		{
			f32 normA = 1.0f;
			f32 normR = (normA * 1.0f);
			f32 normG = (normA * 1.0f);
			f32 normB = (normA * 1.0f);

			u8 r = (u8)(normR * 255.0f);
			u8 g = (u8)(normG * 255.0f);
			u8 b = (u8)(normB * 255.0f);
			u8 a = (u8)(normA * 255.0f);

			u32 color       = (a << 24) | (r << 16) | (g << 8) | (b << 0);
			bitmapBuffer[i] = color;
		}
	}
}

FILE_SCOPE void dchip8_init_display(PlatformRenderBuffer renderBuffer)
{
	// Init screen to 0 alpha, and let alpha simulate "turning on a pixel"
	i32 numPixels     = renderBuffer.width * renderBuffer.height;
	u32 *bitmapBuffer = (u32 *)renderBuffer.memory;
	for (i32 i = 0; i < numPixels; i++)
	{
		u8 r = (u8)(0.0f);
		u8 g = (u8)(0.0f);
		u8 b = (u8)(0.0f);
		u8 a = (u8)(0.0f);

		u32 color       = (a << 24) | (r << 16) | (g << 8) | (b << 0);
		bitmapBuffer[i] = color;
	}
}

FILE_SCOPE Chip8Controller dchip8_controller_map_input(PlatformInput input)
{
	// NOTE: Chip8 Hex Controller to Keyboard Mapping
	// Keypad         Keyboard
	// +-+-+-+-+     +-+-+-+-+
	// |1|2|3|C|     |1|2|3|4|
	// +-+-+-+-+     +-+-+-+-+
	// |4|5|6|D|     |Q|W|E|R|
	// +-+-+-+-+  => +-+-+-+-+
	// |7|8|9|E|     |A|S|D|F|
	// +-+-+-+-+     +-+-+-+-+
	// |A|0|B|F|     |Z|X|C|V|
	// +-+-+-+-+     +-+-+-+-+

	Chip8Controller result = {};

	result.key[0x01] = input.key_1.endedDown;
	result.key[0x02] = input.key_2.endedDown;
	result.key[0x03] = input.key_3.endedDown;
	result.key[0x0C] = input.key_4.endedDown;

	result.key[0x04] = input.key_q.endedDown;
	result.key[0x05] = input.key_w.endedDown;
	result.key[0x06] = input.key_e.endedDown;
	result.key[0x0D] = input.key_r.endedDown;

	result.key[0x07] = input.key_a.endedDown;
	result.key[0x08] = input.key_s.endedDown;
	result.key[0x09] = input.key_d.endedDown;
	result.key[0x0E] = input.key_f.endedDown;

	result.key[0x0A] = input.key_z.endedDown;
	result.key[0x00] = input.key_x.endedDown;
	result.key[0x0B] = input.key_c.endedDown;
	result.key[0x0F] = input.key_v.endedDown;

	return result;
}

void dchip8_update(PlatformRenderBuffer renderBuffer, PlatformInput input,
                   PlatformMemory memory, u32 cyclesToEmulate)
{
	DQNT_ASSERT(cpu.indexRegister >= 0 && cpu.indexRegister <= 0xFFF);
	DQNT_ASSERT(cpu.programCounter >= 0 && cpu.programCounter <= 0xFFF);
	DQNT_ASSERT(renderBuffer.bytesPerPixel == 4);
	DQNT_ASSERT(memory.permanentMemSize == 4096);

	u8 *mainMem                = (u8 *)memory.permanentMem;
	Chip8Controller controller = dchip8_controller_map_input(input);

	if (input.loadNewRom)
	{
		dchip8_init_memory(mainMem, memory.permanentMemSize);
		dchip8_init_cpu(&cpu);
		dchip8_init_display(renderBuffer);

		PlatformFile file = {};
		if (platform_open_file(input.rom, &file))
		{
			DQNT_ASSERT((INIT_ADDRESS + file.size) <= memory.permanentMemSize);

			void *loadToAddr = (void *)(&mainMem[INIT_ADDRESS]);
			if (platform_read_file(file, loadToAddr, (u32)file.size))
			{
				cpu.state = chip8state_running;
			}
			else
			{
				cpu.state = chip8state_off;
			}
			platform_close_file(&file);
		}

		input.loadNewRom = false;
	}

	if (cpu.state == chip8state_await_input)
	{
		for (i32 keyVal = 0; keyVal < DQNT_ARRAY_COUNT(controller.key); keyVal++)
		{
			if (controller.key[keyVal])
			{
				u8 regIndex = cpu.storeKeyToRegisterIndex;
				DQNT_ASSERT(keyVal >= 0 && keyVal <= 0x0F);

				cpu.registerArray[regIndex] = (u8)keyVal;
				cpu.state                   = chip8state_running;
				break;
			}
		}
	}

	if (cpu.state == chip8state_running)
	{
		bool earlyExit = false;
		for (u32 opCycle = 0; opCycle < cyclesToEmulate && !earlyExit;
		     opCycle++)
		{
			u8 opHighByte = mainMem[cpu.programCounter++];
			u8 opLowByte  = mainMem[cpu.programCounter++];
			u8 opFirstNibble = (opHighByte & 0xF0);
			switch (opFirstNibble)
			{
				case 0x00:
				{
					// CLS - 00E0 - Clear the display
					if (opLowByte == 0xE0)
					{
						dchip8_init_display(renderBuffer);
					}
					// RET - 00EE - Return from subroutine
					else if (opLowByte == 0xEE)
					{
						cpu.programCounter = cpu.stack[--cpu.stackPointer];
					}
				}
				break;

				case 0x10:
				case 0x20:
				{
					u16 loc = ((0x0F & opHighByte) << 8) | opLowByte;
					DQNT_ASSERT(loc <= 0x0FFF);

					// JP addr - 1nnn - Jump to location nnn
					if (opFirstNibble == 0x10)
					{
						// NOTE: Jump to loc, as per below
					}
					// Call addr - 2nnn - Call subroutine at nnn
					else
					{
						DQNT_ASSERT(opFirstNibble == 0x20);
						cpu.stack[cpu.stackPointer++] = cpu.programCounter;
						DQNT_ASSERT(cpu.stackPointer <
						            DQNT_ARRAY_COUNT(cpu.stack));
					}

					cpu.programCounter = loc;
				}
				break;

				case 0x30:
				case 0x40:
				{
					u8 regNum = (0x0F & opHighByte);
					DQNT_ASSERT(regNum < DQNT_ARRAY_COUNT(cpu.registerArray));
					u8 *vx = &cpu.registerArray[regNum];

					u8 valToCheck = opLowByte;

					// SE Vx, byte - 3xkk - Skip next instruction if Vx == kk
					if (opFirstNibble == 0x30)
					{
						if (*vx == valToCheck) cpu.programCounter += 2;
					}
					// SNE Vx, byte - 4xkk - Skip next instruction if Vx == kk
					else
					{
						DQNT_ASSERT(opFirstNibble == 0x40);
						if (*vx != valToCheck) cpu.programCounter += 2;
					}
				}
				break;

				// SE Vx, Vy - 5xy0 - Skip next instruction if Vx = Vy
				case 0x50:
				{
					u8 firstRegNum = (0x0F & opHighByte);
					DQNT_ASSERT(firstRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 secondRegNum = (0xF0 & opLowByte) >> 4;
					DQNT_ASSERT(secondRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 *vx = &cpu.registerArray[firstRegNum];
					u8 *vy = &cpu.registerArray[secondRegNum];

					if (*vx == *vy) cpu.programCounter += 2;
				}
				break;

				case 0x60:
				case 0x70:
				{
					u8 regNum = (0x0F & opHighByte);
					DQNT_ASSERT(regNum < DQNT_ARRAY_COUNT(cpu.registerArray));
					u8 valToOperateOn = opLowByte;

					u8 *vx = &cpu.registerArray[regNum];
					// LD Vx, byte - 6xkk - Set Vx = kk
					if (opFirstNibble == 0x60)
					{
						*vx = valToOperateOn;
					}
					// ADD Vx, byte - 7xkk - Set Vx = Vx + kk
					else
					{
						DQNT_ASSERT(opFirstNibble == 0x70);
						*vx += valToOperateOn;
					}
				}
				break;

				case 0x80:
				{
					u8 firstRegNum = (0x0F & opHighByte);
					DQNT_ASSERT(firstRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 secondRegNum = (0xF0 & opLowByte) >> 4;
					DQNT_ASSERT(secondRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 *vx = &cpu.registerArray[firstRegNum];
					u8 *vy = &cpu.registerArray[secondRegNum];

					u8 opFourthNibble = (opLowByte & 0x0F);
					// LD Vx, Vy - 8xy0 - Set Vx = Vy
					if (opFourthNibble == 0x00)
					{
						*vx = *vy;
					}
					// OR Vx, Vy - 8xy1 - Set Vx = Vx OR Vy
					else if (opFourthNibble == 0x01)
					{
						u8 result = (*vx | *vy);
						*vx       = result;
					}
					// AND Vx, Vy - 8xy2 - Set Vx = Vx AND Vy
					else if (opFourthNibble == 0x02)
					{
						u8 result = (*vx & *vy);
						*vx       = result;
					}
					// XOR Vx, Vy - 8xy3 - Set Vx = Vx XOR Vy
					else if (opFourthNibble == 0x03)
					{
						u8 result = (*vx ^ *vy);
						*vx       = result;
					}
					// ADD Vx, Vy - 8xy4 - Set Vx = Vx + Vy, set VF = carry
					else if (opFourthNibble == 0x04)
					{
						u16 result = (*vx + *vy);
						*vx = (result > 255) ? (u8)(result - 256) : (u8)result;

						cpu.VF = (result > 255) ? 1 : 0;
					}
					// SUB Vx, Vy - 8xy5 - Set Vx = Vx - Vy, set VF = NOT borrow
					else if (opFourthNibble == 0x05)
					{
						if (*vx > *vy)
						{
							cpu.VF = 1;
							*vx -= *vy;
						}
						else
						{
							cpu.VF = 0;
							*vx    = (u8)(256 + *vx - *vy);
						}
					}
					// SHR Vx {, Vy} - 8xy6 - Set Vx = Vx SHR 1
					else if (opFourthNibble == 0x06)
					{
						if (*vx & 1)
							cpu.VF = 1;
						else
							cpu.VF = 0;

						*vx >>= 1;
					}
					// SUBN Vx {, Vy} - 8xy7 - Set Vx = Vy - Vx, set VF = NOT
					// borrow
					else if (opFourthNibble == 0x07)
					{
						if (*vy > *vx)
						{
							cpu.VF = 1;
							*vx    = *vy - *vx;
						}
						else
						{
							cpu.VF = 0;
							*vx    = (u8)(256 + *vy - *vx);
						}
					}
					// SHL Vx {, Vy} - 8xyE - Set Vx = SHL 1
					else
					{
						DQNT_ASSERT(opFourthNibble == 0x0E);
						if ((*vx >> 7) == 1)
							cpu.VF = 1;
						else
							cpu.VF = 0;

						*vx <<= 1;
					}
				}
				break;

				// SNE Vx, Vy - 9xy0 - Skip next instruction if Vx != Vy
				case 0x90:
				{
					u8 firstRegNum = (0x0F & opHighByte);
					DQNT_ASSERT(firstRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 secondRegNum = (0xF0 & opLowByte) >> 4;
					DQNT_ASSERT(secondRegNum <
					            DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 *vx = &cpu.registerArray[firstRegNum];
					u8 *vy = &cpu.registerArray[secondRegNum];

					if (*vx != *vy) cpu.programCounter += 2;
				}
				break;

				// LD I, addr - Annn - Set I = nnn
				case 0xA0:
				{
					u16 valToSet      = ((0x0F & opHighByte) << 8) | opLowByte;
					cpu.indexRegister = valToSet;
				}
				break;

				// JP V0, addr - Bnnn - Jump to location (nnn + V0)
				case 0xB0:
				{
					u16 addr =
					    (((0x0F & opHighByte) << 8) | opLowByte) + cpu.V0;
					cpu.programCounter = addr;
				}
				break;

				// RND Vx, byte - Cxkk - Set Vx = random byte AND kk
				case 0xC0:
				{
					u8 regNum = (0x0F & opHighByte);
					DQNT_ASSERT(regNum < DQNT_ARRAY_COUNT(cpu.registerArray));
					u8 *vx = &cpu.registerArray[regNum];

					u8 randNum = (u8)dqnt_rnd_pcg_range(&pcgState, 0, 255);
					u8 andBits = opLowByte;
					DQNT_ASSERT(randNum >= 0 && randNum <= 255);

					*vx = (randNum & andBits);
				}
				break;

				// DRW Vx, Vy, nibble - Dxyn - Display n-byte sprite starting at
				// mem
				// location I at (Vx, Vy), set VF = collision
				case 0xD0:
				{
					u8 xRegister = (0x0F & opHighByte);
					u8 yRegister = (0xF0 & opLowByte) >> 4;
					DQNT_ASSERT(
					    xRegister < DQNT_ARRAY_COUNT(cpu.registerArray) &&
					    yRegister < DQNT_ARRAY_COUNT(cpu.registerArray));

					u8 initPosX = cpu.registerArray[xRegister];
					u8 initPosY = cpu.registerArray[yRegister];

					u8 readNumBytesFromMem = (0x0F & opLowByte);
					// NOTE: can't be more than 16 in Y according to specs.
					DQNT_ASSERT(readNumBytesFromMem < 16);

					u8 *renderBitmap          = (u8 *)renderBuffer.memory;
					const i32 BYTES_PER_PIXEL = renderBuffer.bytesPerPixel;
					i32 pitch = renderBuffer.width * BYTES_PER_PIXEL;

					bool collisionFlag = false;
					for (i32 i = 0; i < readNumBytesFromMem; i++)
					{
						u8 spriteBytes = mainMem[cpu.indexRegister + i];
						u8 posY        = initPosY + (u8)i;
						if (posY >= renderBuffer.height) posY = 0;

						// NOTE: Flip the Y
						posY = ((u8)(renderBuffer.height - 1) - posY);

						const i32 ALPHA_BYTE_INTERVAL =
						    renderBuffer.bytesPerPixel;
						const i32 BITS_IN_BYTE = 8;
						i32 baseBitShift       = BITS_IN_BYTE - 1;
						for (i32 shift = 0; shift < BITS_IN_BYTE; shift++)
						{
							u8 posX = initPosX + (u8)shift;

							if (posX >= renderBuffer.width) posX = 0;
							u32 bitmapOffset =
							    (posX * BYTES_PER_PIXEL) + (posY * pitch);

							// NOTE: Since we are using a 4bpp bitmap, let's use
							// the
							// alpha channel to determine if a pixel is on or
							// not.
							u32 *pixel  = (u32 *)(&renderBitmap[bitmapOffset]);
							u8 alphaBit = (*pixel >> 24);

							DQNT_ASSERT(alphaBit == 0 || alphaBit == 255);
							bool pixelWasOn = (alphaBit == 255) ? true : false;

							i32 bitShift   = baseBitShift - shift;
							bool spriteBit = ((spriteBytes >> bitShift) & 1);
							bool pixelIsOn = (pixelWasOn ^ spriteBit);

							// NOTE: If caused a pixel to XOR into off, then
							// this is
							// known as a "collision" in chip8
							if (pixelWasOn && !pixelIsOn) collisionFlag = true;

							if (pixelIsOn)
							{
								*pixel = 0xFFFFFFFF;
							}
							else
							{
								*pixel = 0;
							}
						}
					}

					cpu.VF = collisionFlag;
				}
				break;

				case 0xE0:
				{
					u8 regNum = (0x0F & opHighByte);
					DQNT_ASSERT(regNum < DQNT_ARRAY_COUNT(cpu.registerArray));
					u8 vx = cpu.registerArray[regNum];
					DQNT_ASSERT(vx >= 0 && vx <= 0x0F);
					DQNT_ASSERT(vx < DQNT_ARRAY_COUNT(controller.key));

					bool skipNextInstruction = false;
					// SKP Vx - Ex9E - Skip next instruction if key with the
					// value
					// of Vx is pressed
					if (opLowByte == 0x9E)
					{
						skipNextInstruction = controller.key[vx];
					}
					// SKNP Vx - ExA1 - Skip next instruction if key with the
					// value
					// of Vx is not pressed
					else
					{
						DQNT_ASSERT(opLowByte == 0xA1);
						skipNextInstruction = !controller.key[vx];
					}

					if (skipNextInstruction) cpu.programCounter += 2;
				}
				break;

				case 0xF0:
				{
					u8 regNum = (0x0F & opHighByte);
					DQNT_ASSERT(regNum < DQNT_ARRAY_COUNT(cpu.registerArray));
					u8 *vx = &cpu.registerArray[regNum];

					// LD Vx, DT - Fx07 - Set Vx = delay timer value
					if (opLowByte == 0x07)
					{
						*vx = cpu.delayTimer;
					}
					// LD Vx, K - Fx0A - Wait for a key press, store the value
					// of
					// the key in Vx
					else if (opLowByte == 0x0A)
					{
						cpu.state                   = chip8state_await_input;
						cpu.storeKeyToRegisterIndex = regNum;
						earlyExit = true;
					}
					// LD DT, Vx - Fx15 - Set delay timer = Vx
					else if (opLowByte == 0x15)
					{
						cpu.delayTimer = *vx;
					}
					// LD ST, Vx - Fx18 - Set sound timer = Vx
					else if (opLowByte == 0x18)
					{
						cpu.soundTimer = *vx;
					}
					// ADD I, Vx - Fx1E - Set I = I + Vx
					else if (opLowByte == 0x1E)
					{
						cpu.indexRegister += *vx;
					}
					// LD F, Vx - Fx29 - Set I = location of sprite for digit Vx
					else if (opLowByte == 0x29)
					{
						u8 hexCharFromFontSet = *vx;
						DQNT_ASSERT(hexCharFromFontSet >= 0x00 &&
						            hexCharFromFontSet <= 0x0F);

						const u32 START_ADDR_OF_FONT = 0;
						const u32 BYTES_PER_FONT     = 5;

						cpu.I = START_ADDR_OF_FONT +
						        (hexCharFromFontSet * BYTES_PER_FONT);
					}
					// LD B, Vx - Fx33 - Store BCD representations of Vx in
					// memory
					// locations I, I+1 and I+2
					else if (opLowByte == 0x33)
					{
						DQNT_ASSERT(regNum <
						            DQNT_ARRAY_COUNT(cpu.registerArray));
						u8 vxVal = *vx;

						const i32 NUM_DIGITS_IN_HUNDREDS = 3;
						for (i32 i = 0; i < NUM_DIGITS_IN_HUNDREDS; i++)
						{
							u8 rem = vxVal % 10;
							vxVal /= 10;

							mainMem[cpu.I +
							        ((NUM_DIGITS_IN_HUNDREDS - 1) - i)] = rem;
						}
					}
					// LD [I], Vx - Fx55 - Store register V0 through Vx in
					// memory
					// starting at location I.
					else if (opLowByte == 0x55)
					{
						for (u32 regIndex = 0; regIndex <= regNum; regIndex++)
						{
							u32 mem_offset = regIndex;
							mainMem[cpu.indexRegister + mem_offset] =
							    cpu.registerArray[regIndex];
						}
					}
					// LD [I], Vx - Fx65 - Read registers V0 through Vx from
					// memory
					// starting at location I.
					else
					{
						DQNT_ASSERT(opLowByte == 0x65);
						for (u32 regIndex = 0; regIndex <= regNum; regIndex++)
						{
							u32 mem_offset = regIndex;
							cpu.registerArray[regIndex] =
							    mainMem[cpu.indexRegister + mem_offset];
						}
					}
				}
				break;
			};
		}

		// IMPORTANT: Timers need to be decremented at a rate of 60hz. Since we
		// can run the interpreter faster than that, make sure we decrement
		// timers at the fixed rate.
		if (cpu.delayTimer > 0 || cpu.soundTimer > 0)
		{
			cpu.elapsedTime += input.deltaForFrame;
			f32 TIMER_DECREMENT_INTERVAL = 1 / 60.0f;

			if (cpu.elapsedTime >= TIMER_DECREMENT_INTERVAL)
			{
				cpu.elapsedTime = 0;
				if (cpu.delayTimer > 0) cpu.delayTimer--;

				if (cpu.soundTimer > 0)
				{
					cpu.soundTimer--;
					if (cpu.soundTimer == 1)
					{
						// TODO(doyle): This needs to play a buzzing sound
						// whilst timer > 0
					}
				}
			}
		}
		else
		{
			cpu.elapsedTime = 0;
		}
	}

}
