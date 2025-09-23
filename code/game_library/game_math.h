
struct v2s
{
    s32 X, Y;
};

struct v4
{
    r32 X, Y, Z, W;
};

struct matrix3x3
{
    r32 m[3][3];
};

matrix3x3 operator*(const matrix3x3& m1, const matrix3x3& m2)
{
    return
    {
        m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0],
        m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1],
        m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2],
        m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0],
        m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1],
        m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2],
        m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0],
        m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1],
        m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2]
    };
}

#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

#ifndef PI
#define PI 3.141592653589793238462f
#endif

// Both Min and Max inclusive.
inline b32
CircularIndexIsInRange(s32 Index, s32 RangeMin, s32 RangeMax){
	b32 Result;
	if (RangeMin <= RangeMax){
		Result = (Index >= RangeMin && Index <= RangeMax);
	}else{
		Result = (Index >= RangeMin || Index <= RangeMax);
	}
	return Result;
}

// Returns the positive number you'd have to add to IndexB to get to IndexA, if they
// wrap by Mod.
inline s32
CircularIndexDifference(s32 IndexA, s32 IndexB, s32 Mod){
	s32 Result = IndexA - IndexB;
	if (Result < 0)
		Result += Mod;

	Assert(Result >= 0 && Result < Mod);
	return Result;
}

f32
SafeDivide(f32 Numerator, f32 Denominator, f32 Default)
{
	f32 Result = (Denominator ? Numerator/Denominator : Default);
	return Result;
}
f32 SafeDivide0(f32 Numerator, f32 Denominator) { return SafeDivide(Numerator, Denominator, 0); }
f32 SafeDivide1(f32 Numerator, f32 Denominator) { return SafeDivide(Numerator, Denominator, 1); }


// MARK: Basic Functions

f32
Frac(f32 Value)
{
	f32 Result = Value - (f32)((s64)Value);
	return Result;
}

inline f32
Square(f32 X)
{
	f32 Result = X*X;
	return Result;
}

inline f32
Sign(f32 X)
{
	f32 Result;
	if (X > 0.0f) Result =  1.f;
	else if (X)   Result = -1.f;
	else		  Result =  0.f;
	return Result;
}

inline f32
Min(f32 A, f32 B)
{
	return (A < B ? A : B);
}

inline f32
Max(f32 A, f32 B)
{
	return (A > B ? A : B);
}

inline f32
Clamp(f32 X, f32 Min, f32 Max)
{
	if (X < Min) return Min;
	if (X > Max) return Max;
	return X;
}
inline s32
ClampS32(s32 X, s32 Min, s32 Max)
{
	if (X < Min) return Min;
	if (X > Max) return Max;
	return X;
}

inline f32
Clamp01(f32 X)
{
	return Clamp(X, 0, 1);
}

inline f32
Lerp(f32 A, f32 B, f32 T)
{
	f32 Result = (1.0f-T)*A + T*B;
	return Result;
}
 
inline f32
LerpClamp(f32 A, f32 B, f32 T)
{
	f32 Result = Lerp(A, B, Clamp01(T));
	return Result;
}

inline f32
Abs(f32 X)
{
	f32 Result = (X < 0 ? -X : X);
	return Result;
}

inline f32
MoveValueTowards(f32 A, f32 B, f32 Amount)
{
	f32 Result = A;
	if (B < A) {
		Result = MAX(B, A - Amount);
	}else{
		Result = MIN(B, A + Amount);
	}
	return Result;
}

// MARK: V2s
inline v2s V2S(s32 X, s32 Y)
{
    v2s Result;
    Result.X = X;
    Result.Y = Y;
    return (Result);
}

// MARK: V2
union v2
{
    struct
    {
        r32 X, Y;
    };
    r32 E[2];
};

inline v2 V2(r32 X, r32 Y)
{
    v2 Result;

    Result.X = X;
    Result.Y = Y;

    return (Result);
}

inline v2 
operator*(r32 A, v2 B)
{
    v2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

    return (Result);
}

inline v2 
operator*(v2 A, r32 B)
{
    v2 Result;

    Result.X = A.X*B;
    Result.Y = A.Y*B;

    return (Result);
}

inline v2 
operator/(v2 A, r32 Scalar)
{
	v2 Result;
	
	Result.X = A.X/Scalar;
	Result.Y = A.Y/Scalar;
    
	return (Result);
}

inline v2 
operator-(v2 A)
{
    v2 Result;

    Result.X = -A.X;
    Result.Y = -A.Y;

    return (Result);
}

inline v2 
operator+(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return (Result);
}

inline v2 
operator-(v2 A, v2 B)
{
    v2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return (Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return (A);
}

inline v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return (A);
}

inline v2 &
operator*=(v2 &A, r32 Scalar)
{
    A = A*Scalar;
    return (A);
}

inline v2 &
operator/=(v2 &A, r32 Scalar)
{
    A = A/Scalar;
    return (A);
}

b32 
v2SIsEqual(v2s A, v2s B)
{
    if (A.X == B.X &&
        A.Y == B.Y)
    {
        return true;
    }

    return false;
}

struct rectangle2
{
    v2 Min;
    v2 Max;
};

rectangle2
Rectangle2(v2 Min, v2 Max)
{
    rectangle2 Result = {};
    Result.Min = Min;
    Result.Max = Max;
    return Result;
}

rectangle2
IncrementedXByLength(rectangle2 Original, r32 Length)
{
    rectangle2 Result = Original;
    Result.Min.X += Length;
    Result.Max.X += Length;
    return Result;
}

rectangle2
IncrementedYByHeight(rectangle2 Original, r32 Height)
{
    rectangle2 Result = Original;
    Result.Min.Y += Height;
    Result.Max.Y += Height;
    return Result;
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.X*B.X + A.Y*B.Y;
    
    return (Result);
}

inline r32
LengthSQ(v2 A)
{
    r32 Result = Inner(A, A);

    return (Result);
}

b32 ValueIsInRange(u32 Value, u32 Lower, u32 Upper)
{
    if (Value >= Lower &&
        Value <= Upper)
    {
        return true;
    }

    return false;
}

// MARK: Random

// Return range: [0, 1]
inline f32
Random01()
{
	s32 RandomInt = rand();
	f32 Result = (f32)RandomInt/(f32)RAND_MAX;
	return Result;
}
// Return range: [Min, Max]
inline f32
RandomRange(f32 Min, f32 Max)
{
	f32 Result = Min + Random01()*(Max - Min);
	return Result;
}
// Return range: [0, Max]
inline f32
Random(f32 Max)
{
	f32 Result = Random01()*Max;
	return Result;
}

// Return range: [0, Max]
inline s32
RandomS32(s32 Max)
{
	s32 Result = (s32)(Random01()*(f32)Max + .5f);
	return Result;
}
// Return range: [Min, Max]
inline s32
RandomRangeS32(s32 Min, s32 Max)
{
	s32 Result = Min + (s32)(Random01()*(f32)(Max - Min) + .5f);
	return Result;
}

b32 
PointIsInsideRectangle(v2 Point, rectangle2 Rectangle)
{
    if (Point.X >= Rectangle.Min.X &&
        Point.X <= Rectangle.Max.X &&
        Point.Y >= Rectangle.Min.Y &&
        Point.Y <= Rectangle.Max.Y)
    {
        return true;
    }

    return false;
}
