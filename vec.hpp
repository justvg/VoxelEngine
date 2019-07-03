#pragma once

union v2
{
	struct
	{
		real32 x, y;
	};
	struct
	{
		real32 u, v;
	};
	real32 E[2];

	void Normalize();
};

union v3
{
	struct
	{
		real32 x, y, z;
	};
	struct
	{
		real32 u, v, w;
	};
	struct
	{
		real32 r, g, b;
	};
	struct
	{
		v2 xy;
		real32 Ignored_0;
	};
	struct
	{
		real32 Ignored_1;
		v2 yz;
	};
	struct
	{
		v2 uv;
		real32 Ignored_2;
	};
	struct
	{
		real32 Ignored_3;
		v2 vw;
	};
	real32 E[3];

	void Normalize();
};

union v4
{
	struct
	{
		real32 x, y, z, w;
	};
	struct
	{
		real32 r, g, b, a;
	};
	struct
	{
		v3 xyz;
		real32 Ignored_0;
	};
	struct
	{
		v3 rgb;
		real32 Ignored_1;
	};
	struct
	{
		v2 xy;
		real32 Ignored_2;
		real32 Ignored_3;
	};
	struct
	{
		real32 Ignored_4;
		v2 yz;
		real32 Ignored_5;
	};
	struct
	{
		real32 Ignored_6;
		real32 Ignored_7;
		v2 zw;
	};
	real32 E[4];

	void Normalize();
};

inline v2
V2(real32 X, real32 Y)
{
	v2 Result;

	Result.x = X;
	Result.y = Y;

	return(Result);
}

inline v3
V3(real32 X, real32 Y, real32 Z)
{
	v3 Result;

	Result.x = X;
	Result.y = Y;
	Result.z = Z;

	return(Result);
}

inline v4
V4(real32 X, real32 Y, real32 Z, real32 W)
{
	v4 Result;

	Result.x = X;
	Result.y = Y;
	Result.z = Z;
	Result.w = W;

	return(Result);
}

inline v4
V4(v3 A, real32 W)
{
	v4 Result;

	Result.x = A.x;
	Result.y = A.y;
	Result.z = A.z;
	Result.w = A.w;

	return(Result);
}

//
// NOTE(georgy): v2 operations
//

inline v2
operator*(real32 A, v2 B)
{
	v2 Result;

	Result.x = A * B.x;
	Result.y = A * B.y;

	return(Result);
}

inline v2
operator*(v2 B, real32 A)
{
	v2 Result = A * B;

	return(Result);
}

inline v2 &
operator*=(v2 &A, real32 B)
{
	A = B * A;

	return(A);
}

inline v2
operator-(v2 A)
{
	v2 Result;

	Result.x = -A.x;
	Result.y = -A.y;

	return(Result);
}

inline v2
operator+(v2 A, v2 B)
{
	v2 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;

	return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
	A = A + B;

	return(A);
}

inline v2
operator-(v2 A, v2 B)
{
	v2 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;

	return(Result);
}

inline v2 &
operator-=(v2 &A, v2 B)
{
	A = A - B;

	return(A);
}

inline v2
Hadamard(v2 A, v2 B)
{
	v2 Result;

	Result.x = A.x * B.x;
	Result.y = A.y * B.y;

	return(Result);
}

inline real32
Dot(v2 A, v2 B)
{
	real32 Result = A.x*B.x + A.y*B.y;

	return(Result);
}

inline real32
LengthSq(v2 A)
{
	real32 Result = Dot(A, A);

	return(Result);
}

inline real32
Length(v2 A)
{
	real32 Result = sqrtf(LengthSq(A));

	return(Result);
}

inline v2
Normalize(v2 A)
{
	v2 Result;

	real32 InvLen = 1.0f / Length(A);
	Result = InvLen * A;

	return(Result);
}

inline void
v2::Normalize()
{
	real32 InvLen = 1.0f / Length(*this);

	*this *= InvLen;
}

//
// NOTE(georgy): v3 operations
//

inline v3
operator*(real32 A, v3 B)
{
	v3 Result;

	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;

	return(Result);
}

inline v3
operator*(v3 B, real32 A)
{
	v3 Result = A * B;

	return(Result);
}

inline v3 &
operator*=(v3 &A, real32 B)
{
	A = B * A;

	return(A);
}

inline v3
operator-(v3 A)
{
	v3 Result;

	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;

	return(Result);
}

inline v3
operator+(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;

	return(Result);
}

inline v3 &
operator+=(v3 &A, v3 B)
{
	A = A + B;

	return(A);
}

inline v3
operator-(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;

	return(Result);
}

inline v3 &
operator-=(v3 &A, v3 B)
{
	A = A - B;

	return(A);
}

inline v3
Hadamard(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;

	return(Result);
}

inline real32
Dot(v3 A, v3 B)
{
	real32 Result = A.x*B.x + A.y*B.y + A.z*B.z;

	return(Result);
}

inline real32
LengthSq(v3 A)
{
	real32 Result = Dot(A, A);

	return(Result);
}

inline real32
Length(v3 A)
{
	real32 Result = sqrtf(LengthSq(A));

	return(Result);
}

inline v3
Cross(v3 A, v3 B)
{
	v3 Result;

	Result.x = A.y*B.z - A.z*B.y;
	Result.y = A.z*B.x - A.x*B.z;
	Result.z = A.x*B.y - A.y*B.x;

	return(Result);
}

inline v3
Normalize(v3 A)
{
	v3 Result;

	real32 InvLen = 1.0f / Length(A);
	Result = InvLen * A;

	return(Result);
}

inline void
v3::Normalize()
{
	real32 InvLen = 1.0f / Length(*this);

	*this *= InvLen;
}

//
// NOTE(georgy): v4 operations
//

inline v4
operator*(real32 A, v4 B)
{
	v4 Result;

	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;
	Result.w = A * B.w;

	return(Result);
}

inline v4
operator*(v4 B, real32 A)
{
	v4 Result = A * B;

	return(Result);
}

inline v4 &
operator*=(v4 &A, real32 B)
{
	A = B * A;

	return(A);
}

inline v4
operator-(v4 A)
{
	v4 Result;

	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	Result.w = -A.w;

	return(Result);
}

inline v4
operator+(v4 A, v4 B)
{
	v4 Result;

	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;

	return(Result);
}

inline v4 &
operator+=(v4 &A, v4 B)
{
	A = A + B;

	return(A);
}

inline v4
operator-(v4 A, v4 B)
{
	v4 Result;

	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;

	return(Result);
}

inline v4 &
operator-=(v4 &A, v4 B)
{
	A = A - B;

	return(A);
}

inline v4
Hadamard(v4 A, v4 B)
{
	v4 Result;

	Result.x = A.x * B.x;
	Result.y = A.y * B.y;
	Result.z = A.z * B.z;
	Result.w = A.w * B.w;

	return(Result);
}

inline real32
Dot(v4 A, v4 B)
{
	real32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;

	return(Result);
}

inline real32
LengthSq(v4 A)
{
	real32 Result = Dot(A, A);

	return(Result);
}

inline real32
Length(v4 A)
{
	real32 Result = sqrtf(LengthSq(A));

	return(Result);
}

inline v4
Normalize(v4 A)
{
	v4 Result;

	real32 InvLen = 1.0f / Length(A);
	Result = InvLen * A;

	return(Result);
}

inline void
v4::Normalize()
{
	real32 InvLen = 1.0f / Length(*this);

	*this *= InvLen;
}