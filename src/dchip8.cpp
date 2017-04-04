#include "common.h"
#include "dchip8_platform.h"

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

	bool isInit;
} Chip8CPU;

FILE_SCOPE Chip8CPU cpu;
FILE_SCOPE u16      opCodes[35];

void dchip8_update(PlatformRenderBuffer renderBuffer, PlatformInput input,
                   PlatformMemory memory)
{
	ASSERT(indexRegister >= 0 && indexRegister <= 0xFFF);
	ASSERT(programCounter >= 0 && programCounter <= 0xFFF);

	ASSERT(renderBuffer.bytesPerPixel == 4);

	const i32 numPixels = renderBuffer.width * renderBuffer.height;
	u32 *bitmapBuffer   = (u32 *)renderBuffer.memory;
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

	if (!cpu.isInit)
	{
		ASSERT(memory.permanentMemSize == (4096 / 4));
		cpu.isInit = true;

		// NOTE: Everything before 0x200 is reserved for the actual emulator
		const u32 INIT_ADDRESS = 0x200;
		cpu.programCounter     = INIT_ADDRESS;
		cpu.I                  = 0;
		cpu.stackPointer       = 0;
	}

	u8 *mainMem   = (u8 *)memory.permanentMem;
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
			}
			// RET - 00EE - Return from subroutine
			else if (opLowByte == 0xEE)
			{
				cpu.programCounter = cpu.stack[cpu.stackPointer--];
			}
		}
		break;

		case 0x10:
		case 0x20:
		{
			u16 loc = ((0x0F & opHighByte) << 8) | (0xFF & opLowByte);
			ASSERT(loc <= 0x0FFF);

			// JP addr - 1nnn - Jump to location nnn
			if (opFirstNibble == 0x10)
			{
				// NOTE: Jump to loc, as per below
			}
			// Call addr - 2nnn - Call subroutine at nnn
			else
			{
				ASSERT(opFirstNibble == 0x20);

				cpu.stackPointer++;
				ASSERT(cpu.stackPointer < ARRAY_COUNT(cpu.stack));
				cpu.stack[cpu.stackPointer] = cpu.programCounter;
			}

			cpu.programCounter = loc;
		}
		break;

		case 0x30:
		case 0x40:
		{
			u8 regNum = (0x0F & opHighByte);
			ASSERT(regNum < ARRAY_COUNT(cpu.registerArray));
			u8 valToCheck = opLowByte;

			// SE Vx, byte - 3xkk - Skip next instruction if Vx == kk
			if (opFirstNibble == 0x30)
			{
				if (cpu.registerArray[regNum] == valToCheck)
					cpu.programCounter += 2;
			}
			// SNE Vx, byte - 4xkk - Skip next instruction if Vx == kk
			else
			{
				ASSERT(opFirstNibble == 0x40);
				if (cpu.registerArray[regNum] != valToCheck)
					cpu.programCounter += 2;
			}
		}
		break;

		// SE Vx, Vy - 5xy0 - Skip next instruction if Vx = Vy
		case 0x50:
		{
			u8 firstRegNum = (0x0F & opHighByte);
			ASSERT(firstRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 secondRegNum = (0xF0 & opLowByte);
			ASSERT(secondRegNum < ARRAY_COUNT(cpu.registerArray));

			if (cpu.registerArray[firstRegNum] ==
			    cpu.registerArray[secondRegNum])
			{
				cpu.programCounter++;
			}
		}
		break;

		case 0x60:
		case 0x70:
		{
			u8 regNum = (0x0F & opHighByte);
			ASSERT(regNum < ARRAY_COUNT(cpu.registerArray));
			u8 valToOperateOn = opLowByte;

			// LD Vx, byte - 6xkk - Set Vx = kk
			if (opFirstNibble == 0x60)
			{
				cpu.registerArray[regNum] = valToOperateOn;
			}
			// ADD Vx, byte - 7xkk - Set Vx = Vx + kk
			else
			{
				ASSERT(opFirstNibble == 0x70);
				cpu.registerArray[regNum] += valToOperateOn;
			}

		}
		break;

		case 0x80:
		{
			u8 firstRegNum = (0x0F & opHighByte);
			ASSERT(firstRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 secondRegNum = (0xF0 & opLowByte);
			ASSERT(secondRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 *vx = &cpu.registerArray[firstRegNum];
			u8 *vy = &cpu.registerArray[secondRegNum];

			// LD Vx, Vy - 8xy0 - Set Vx = Vy
			if (opLowByte == 0x00)
			{
				*vx = *vy;
			}
			// OR Vx, Vy - 8xy1 - Set Vx = Vx OR Vy
			else if (opLowByte == 0x01)
			{
				u8 result = (*vx | *vy);
				*vx       = result;
			}
			// AND Vx, Vy - 8xy2 - Set Vx = Vx AND Vy
			else if (opLowByte == 0x02)
			{
				u8 result = (*vx & *vy);
				*vx       = result;
			}
			// XOR Vx, Vy - 8xy3 - Set Vx = Vx XOR Vy
			else if (opLowByte == 0x03)
			{
				u8 result = (*vx & *vy);
				*vx       = result;
			}
			// ADD Vx, Vy - 8xy4 - Set Vx = Vx + Vy, set VF = carry
			else if (opLowByte == 0x04)
			{
				u16 result = (*vx + *vy);
				*vx        = (u8)result;

				if (result > 255) cpu.VF = (result > 255) ? 1 : 0;

			}
			// SUB Vx, Vy - 8xy5 - Set Vx = Vx - Vy, set VF = NOT borrow
			else if (opLowByte == 0x05)
			{
				if (*vx > *vy)
					cpu.VF = 1;
				else
					cpu.VF = 0;

				*vx -= *vy;
			}
			// SHR Vx {, Vy} - 8xy6 - Set Vx = Vx SHR 1
			else if (opLowByte == 0x06)
			{
				if (*vx & 1)
					cpu.VF = 1;
				else
					cpu.VF = 0;

				*vx >>= 1;
			}
			// SUBN Vx {, Vy} - 8xy7 - Set Vx = Vy - Vx, set VF = NOT borrow
			else if (opLowByte == 0x07)
			{
				if (*vy > *vx)
					cpu.VF = 1;
				else
					cpu.VF = 0;

				*vx = *vy - *vx;
			}
			// SHL Vx {, Vy} - 8xyE - Set Vx = SHL 1
			else
			{
				ASSERT(opLowByte == 0x0E);
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
			ASSERT(firstRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 secondRegNum = (0xF0 & opLowByte);
			ASSERT(secondRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 *vx = &cpu.registerArray[firstRegNum];
			u8 *vy = &cpu.registerArray[secondRegNum];

			if (*vx != *vy) cpu.programCounter+= 2;
		}
		break;

		// LD I, addr - Annn - Set I = nnn
		case 0xA0:
		{
			u8 valToSet       = opLowByte;
			cpu.indexRegister = valToSet;
		}
		break;

		// JP V0, addr - Bnnn - Jump to location (nnn + V0)
		case 0xB0:
		{
			u8 addr            = opLowByte + cpu.V0;
			cpu.programCounter = addr;
		}
		break;

		// RND Vx, byte - Cxkk - Set Vx = random byte AND kk
		case 0xC0:
		{
			u8 firstRegNum = (0x0F & opHighByte);
			ASSERT(firstRegNum < ARRAY_COUNT(cpu.registerArray));

			u8 andBits = opLowByte;
			u8 *vx     = &cpu.registerArray[firstRegNum];

			// TODO(doyle): Random umber
			*vx = (28 & opLowByte);
		}
		break;

		// DRW Vx, Vy, nibble - Dxyn - Display n-byte sprite starting at mem
		// location I at (Vx, Vy), set VF = collision
		case 0xD0:
		{
			// TODO(doyle): Implement drawing
		}
		break;

		case 0xE0:
		{
			// TODO(doyle): Implement key checks
			u8 checkKey = (0x0F & opHighByte);

			// SKP Vx - Ex9E - Skip next instruction if key with the value of Vx
			// is pressed
			bool skipNextInstruction = false;
			if (opLowByte == 0x9E)
			{
			}
			// SKNP Vx - ExA1 - Skip next instruction if key with the value of
			// Vx is not pressed
			else
			{
				ASSERT(opLowByte == 0xA1);
			}

			if (skipNextInstruction) cpu.programCounter += 2;
		}
		break;

		case 0xF0:
		{
			u8 regNum = (0x0F & opHighByte);
			ASSERT(regNum < ARRAY_COUNT(cpu.registerArray));
			u8 *vx = &cpu.registerArray[regNum];

			// LD Vx, DT - Fx07 - Set Vx = delay timer value
			if (opLowByte == 0x07)
			{
				*vx = cpu.delayTimer;
			}
			// LD Vx, K - Fx0A - Wait for a key press, store the value of the
			// key in Vx
			else if (opLowByte == 0x0A)
			{
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
				// TODO: Implement
			}
			// LD B, Vx - Fx33 - Store BCD representations of Vx in memory
			// locations I, I+1 and I+2
			else if (opLowByte == 0x33)
			{
			}
			// LD [I], Vx - Fx55 - Store register V0 through Vx in memory
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
			// LD [I], Vx - Fx65 - Read registers V0 through Vx from memory
			// starting at location I.
			else
			{
				ASSERT(opLowByte == 0x65);
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
