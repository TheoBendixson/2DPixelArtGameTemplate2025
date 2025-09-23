#include <stdint.h> 
#include <stddef.h>

#if SLOW 
#define Assert(Expression) if(!(Expression)) { __builtin_trap(); }
#elif WINDOWS
//#define Assert(Expression) if(!(Expression)) { FatalError("Assertion Failure"); }
#define ASSERT_STRINGIFY_(x) ASSERT_STRINGIFY_2_(x)
#define ASSERT_STRINGIFY_2_(x) #x
#define ASSERT_LINE_STRING_ ASSERT_STRINGIFY_(__LINE__)
#define Assert(Expression) if(!(Expression)) { FatalError("Assertion Failure at " __FILE__ " (line " ASSERT_LINE_STRING_ ")"); }
#else
#define Assert(Expression)
#endif

#if MACOS
#define UNREACHABLE_CODE __builtin_trap();
#else
#define UNREACHABLE_CODE
#endif

#define AssertRange(Min, Value, Max) Assert(((Min) <= (Value)) && ((Value) <= (Max)))

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)

#define SWAP(A, B) { \
	auto Temp = A; \
	A = B; \
	B = Temp; \
}

#define ZeroStruct(Pointer) ZeroSize((void *)(Pointer), sizeof((Pointer)[0]))
inline void ZeroSize(void *Pointer, size_t Size){
	memset(Pointer, 0, Size);
}


#define internal static
#define local_persist static
#define global_variable static

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t memory_index;

typedef int8_t b8;
typedef int16_t b16;
typedef int32_t b32;

typedef float r32;
typedef double r64;
typedef float f32;
typedef double f64;



inline b32 IsLittleEndian()
{
	u8 Test[2] = {1, 0};
	b32 Result = (*((u16 *)Test) == 1);
	return Result;
}

// MARK: Bit flags
#define CheckFlag(flags, bitMask) ((flags) & (bitMask))
#define CheckFlagsOr(flags, bitMask) ((flags) & (bitMask))
#define CheckFlagsAnd(flags, bitMask) (((flags) & (bitMask)) == (bitMask))

#define SetFlags(flags, bitMask) { flags |=  bitMask; }
#define UnsetFlags(flags, bitMask)   { flags &= ~(bitMask); }

#define LITTLE_ENDIAN_TO_HOST(DestPointer, SourcePointer) LittleEndianToHost_(DestPointer, SourcePointer, sizeof(*DestPointer), sizeof(*SourcePointer))
void LittleEndianToHost_(void *Dest, void *Source, s32 Size, s32 Size2)
{
	Assert(Size == Size2);
	Assert(Size % 2 == 0)
	memmove(Dest, Source, Size);

	if (!IsLittleEndian())
	{
		u8 *DestU8 = (u8 *)Dest;
		for(s32 i = 0; i < Size/2; i++)
		{
			SWAP(DestU8[i], DestU8[Size - 1 - i]);
		}
	}
}

#define BIG_ENDIAN_TO_HOST(DestPointer, SourcePointer) BigEndianToHost_(DestPointer, SourcePointer, sizeof(*DestPointer), sizeof(*SourcePointer))
void BigEndianToHost_(void *Dest, void *Source, s32 Size, s32 Size2)
{
	Assert(Size == Size2);
	Assert(Size % 2 == 0);
	memmove(Dest, Source, Size);

	if (IsLittleEndian())
	{
		u8 *DestU8 = (u8 *)Dest;
		for(s32 i = 0; i < Size/2; i++)
		{
			SWAP(DestU8[i], DestU8[Size - 1 - i]);
		}
	}
}

