#ifndef DQNT_H
#define DQNT_H

/*
    #define DQNT_IMPLEMENTATION
    #include "dqnt.h"
 */

////////////////////////////////////////////////////////////////////////////////
// General Purpose Operations
////////////////////////////////////////////////////////////////////////////////
#include "stdint.h"
#define LOCAL_PERSIST static
#define FILE_SCOPE    static

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int32_t i64;
typedef int32_t i32;
typedef int64_t i16;

typedef float f32;

#define DQNT_INVALID_CODE_PATH 0
#define DQNT_ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#define DQNT_ASSERT(expr) if (!(expr)) { (*((i32 *)0)) = 0; }
#define DQNT_MATH_ABS(x) (((x) < 0) ? (-(x)) : (x))

////////////////////////////////////////////////////////////////////////////////
// Vec2
////////////////////////////////////////////////////////////////////////////////
typedef union v2 {
	struct { f32 x, y; };
	struct { f32 w, h; };
} v2;

v2 V2(f32 x, f32 y);
// Create a 2d-vector using ints and typecast to floats
v2 V2i(i32 x, i32 y);

////////////////////////////////////////////////////////////////////////////////
// String Ops
////////////////////////////////////////////////////////////////////////////////
bool  dqnt_char_is_digit   (char c);
bool  dqnt_char_is_alpha   (char c);
bool  dqnt_char_is_alphanum(char c);

i32   dqnt_strcmp (const char *a, const char *b);
// Returns the length without the null terminator
i32   dqnt_strlen (const char *a);
char *dqnt_strncpy(char *dest, const char *src, i32 numChars);

bool  dqnt_str_reverse(char *buf, const i32 bufSize);
i32   dqnt_str_to_i32 (char *const buf, const i32 bufSize);
void  dqnt_i32_to_str (i32 value, char *buf, i32 bufSize);

wchar_t dqnt_wchar_ascii_to_lower(wchar_t character);
i32     dqnt_wstrcmp(const wchar_t *a, const wchar_t *b);
void    dqnt_wstrcat(const wchar_t *a, i32 lenA, const wchar_t *b, i32 lenB, wchar_t *out, i32 outLen);
i32     dqnt_wstrlen(const wchar_t *a);

////////////////////////////////////////////////////////////////////////////////
// PCG (Permuted Congruential Generator) Random Number Generator
////////////////////////////////////////////////////////////////////////////////
typedef struct RandPCGState
{
	u64 state[2];
} RandPCGState;

// Init generator with seed. The generator is not valid until it's been seeded.
void dqnt_rnd_pcg_seed (RandPCGState *pcg, u32 seed);
// Returns a random number N between [0, 0xFFFFFFFF]
u32  dqnt_rnd_pcg_next (RandPCGState *pcg);
// Returns a random float N between [0.0, 1.0f]
f32  dqnt_rnd_pcg_nextf(RandPCGState *pcg);
// Returns a random integer N between [min, max]
i32  dqnt_rnd_pcg_range(RandPCGState *pcg, i32 min, i32 max);

#endif  /* DQNT_H */

#ifdef DQNT_IMPLEMENTATION
#undef DQNT_IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////
// Vec2
////////////////////////////////////////////////////////////////////////////////
v2 V2(f32 x, f32 y)
{
	v2 result = {};
	result.x  = x;
	result.y  = y;

	return result;
}

v2 V2i(i32 x, i32 y)
{
	v2 result = V2((f32)x, (f32)y);
	return result;
}

////////////////////////////////////////////////////////////////////////////////
// String Ops
////////////////////////////////////////////////////////////////////////////////
bool dqnt_char_is_digit(char c)
{
	if (c >= '0' && c <= '9') return true;
	return false;
}

bool dqnt_char_is_alpha(char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
	return false;
}

bool dqnt_char_is_alphanum(char c)
{
	if (dqnt_char_is_alpha(c) || dqnt_char_is_digit(c)) return true;
	return false;
}

i32 dqnt_strcmp(const char *a, const char *b)
{
	if (!a && !b) return -1;
	if (!a) return -1;
	if (!b) return -1;

	while ((*a) == (*b))
	{
		if (!(*a)) return 0;
		a++;
		b++;
	}

	return (((*a) < (*b)) ? -1 : 1);
}

i32 dqnt_strlen(const char *a)
{
	i32 result = 0;
	while (a && a[result]) result++;

	return result;
}

char *dqnt_strncpy(char *dest, const char *src, i32 numChars)
{
	if (!dest) return NULL;
	if (!src)  return dest;

	for (i32 i  = 0; i < numChars; i++)
		dest[i] = src[i];

	return dest;
}

bool dqnt_str_reverse(char *buf, const i32 bufSize)
{
	if (!buf) return false;
	i32 mid = bufSize / 2;

	for (i32 i = 0; i < mid; i++)
	{
		char tmp               = buf[i];
		buf[i]                 = buf[(bufSize - 1) - i];
		buf[(bufSize - 1) - i] = tmp;
	}

	return true;
}

i32 dqnt_str_to_i32(char *const buf, const i32 bufSize)
{
	if (!buf || bufSize == 0) return 0;

	i32 index       = 0;
	bool isNegative = false;
	if (buf[index] == '-' || buf[index] == '+')
	{
		if (buf[index] == '-') isNegative = true;
		index++;
	}
	else if (!dqnt_char_is_digit(buf[index]))
	{
		return 0;
	}

	i32 result = 0;
	for (i32 i = index; i < bufSize; i++)
	{
		if (dqnt_char_is_digit(buf[i]))
		{
			result *= 10;
			result += (buf[i] - '0');
		}
		else
		{
			break;
		}
	}

	if (isNegative) result *= -1;

	return result;
}

void dqnt_i32_to_str(i32 value, char *buf, i32 bufSize)
{
	if (!buf || bufSize == 0) return;

	if (value == 0)
	{
		buf[0] = '0';
		return;
	}
	
	// NOTE(doyle): Max 32bit integer (+-)2147483647
	i32 charIndex = 0;
	bool negative           = false;
	if (value < 0) negative = true;

	if (negative) buf[charIndex++] = '-';

	i32 val = DQNT_MATH_ABS(value);
	while (val != 0 && charIndex < bufSize)
	{
		i32 rem          = val % 10;
		buf[charIndex++] = (u8)rem + '0';
		val /= 10;
	}

	// NOTE(doyle): If string is negative, we only want to reverse starting
	// from the second character, so we don't put the negative sign at the end
	if (negative)
	{
		dqnt_str_reverse(buf + 1, charIndex - 1);
	}
	else
	{
		dqnt_str_reverse(buf, charIndex);
	}
}

wchar_t dqnt_wchar_ascii_to_lower(wchar_t character)
{
	if (character >= L'A' && character <= L'Z')
	{
		i32 shiftOffset = L'a' - L'A';
		character += (wchar_t)shiftOffset;
	}

	return character;
}

i32 dqnt_wstrcmp(const wchar_t *a, const wchar_t *b)
{
	if (!a && !b) return -1;
	if (!a) return -1;
	if (!b) return -1;

	while ((*a) == (*b))
	{
		if (!(*a)) return 0;
		a++;
		b++;
	}

	return (((*a) < (*b)) ? -1 : 1);
}

void dqnt_wstrcat(const wchar_t *a, i32 lenA, const wchar_t *b, i32 lenB,
                  wchar_t *out, i32 outLen)
{
	DQNT_ASSERT((lenA + lenB) < outLen);

	i32 outIndex = 0;
	for (i32 i          = 0; i < lenA; i++)
		out[outIndex++] = a[i];

	for (i32 i          = 0; i < lenB; i++)
		out[outIndex++] = b[i];

	DQNT_ASSERT(outIndex <= outLen);
}

i32 dqnt_wstrlen(const wchar_t *a)
{
	i32 result = 0;
	while ((*a))
	{
		result++;
		a++;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
// PCG (Permuted Congruential Generator) Random Number Generator
////////////////////////////////////////////////////////////////////////////////
// Public Domain library with thanks to Mattias Gustavsson
// https://github.com/mattiasgustavsson/libs/blob/master/docs/rnd.md

// Convert a randomized u32 value to a float value x in the range 0.0f <= x
// < 1.0f. Contributed by Jonatan Hedborg
FILE_SCOPE f32 dqnt_rnd_f32_normalized_from_u32_internal(u32 value)
{
	u32 exponent = 127;
	u32 mantissa = value >> 9;
	u32 result   = (exponent << 23) | mantissa;
	f32 fresult  = *(f32 *)(&result);
	return fresult - 1.0f;
}

FILE_SCOPE u64 dqnt_rnd_murmur3_avalanche64_internal(u64 h)
{
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccd;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53;
	h ^= h >> 33;
	return h;
}

void dqnt_rnd_pcg_seed(RandPCGState *pcg, u32 seed)
{
	u64 value     = (((u64)seed) << 1ULL) | 1ULL;
	value         = dqnt_rnd_murmur3_avalanche64_internal(value);
	pcg->state[0] = 0U;
	pcg->state[1] = (value << 1ULL) | 1ULL;
	dqnt_rnd_pcg_next(pcg);
	pcg->state[0] += dqnt_rnd_murmur3_avalanche64_internal(value);
	dqnt_rnd_pcg_next(pcg);
}

u32 dqnt_rnd_pcg_next(RandPCGState *pcg)
{
	u64 oldstate   = pcg->state[0];
	pcg->state[0]  = oldstate * 0x5851f42d4c957f2dULL + pcg->state[1];
	u32 xorshifted = (u32)(((oldstate >> 18ULL) ^ oldstate) >> 27ULL);
	u32 rot        = (u32)(oldstate >> 59ULL);
	return (xorshifted >> rot) | (xorshifted << ((-(i32)rot) & 31));
}

f32 dqnt_rnd_pcg_nextf(RandPCGState *pcg)
{
	return dqnt_rnd_f32_normalized_from_u32_internal(dqnt_rnd_pcg_next(pcg));
}

i32 dqnt_rnd_pcg_range(RandPCGState *pcg, i32 min, i32 max)
{
	i32 const range = (max - min) + 1;
	if (range <= 0) return min;
	i32 const value = (i32)(dqnt_rnd_pcg_nextf(pcg) * range);
	return min + value;
}

#endif /* DQNT_IMPLEMENTATION */
