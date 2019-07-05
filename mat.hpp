#pragma once

struct mat4
{
	union
	{
		real32 Elements[16];
		struct
		{
			real32 a11, a21, a31, a41;
			real32 a12, a22, a32, a42;
			real32 a13, a23, a33, a43;
			real32 a14, a24, a34, a44;
		};
	};
};

internal v4
operator*(mat4 A, v4 B)
{

}

internal mat4 
operator*(mat4 A, mat4 B)
{
	mat4 Result;

	for (uint32 I = 0; I < 4; I++)
	{
		for (uint32 J = 0; J < 4; J++)
		{
			real32 Sum = 0.0f;
			for (uint32 E = 0; E < 4; E++)
			{
				Sum += A.Elements[I + E*4] * B.Elements[J*4 + E];
			}
			Result.Elements[I + J*4] = Sum;
		}
	}

	return(Result);
}

inline mat4 
Identity(float Diagonal)
{
	mat4 Result = {};

	Result.a11 = Diagonal;
	Result.a22 = Diagonal;
	Result.a33 = Diagonal;
	Result.a44 = Diagonal;

	return(Result);
}

inline mat4
Translation(v3 Translate)
{
	mat4 Result = Identity(1.0f);

	Result.a14 = Translate.x;
	Result.a24 = Translate.y;
	Result.a34 = Translate.z;

	return(Result);
}

inline mat4
Scale(real32 Scale)
{
	mat4 Result = {};

	Result.a11 = Scale;
	Result.a22 = Scale;
	Result.a33 = Scale;
	Result.a44 = 1.0f;

	return(Result);
}

inline mat4
Scale(v3 Scale)
{
	mat4 Result = {};

	Result.a11 = Scale.x;
	Result.a22 = Scale.y;
	Result.a33 = Scale.z;
	Result.a44 = 1.0f;

	return(Result);
}

internal mat4
Rotate(real32 Angle, v3 Axis)
{
	mat4 Result;

	real32 Rad = Radians(Angle);
	real32 Cos = cosf(Rad);
	real32 Sin = sinf(Rad);
	Axis.Normalize();

	Result.a11 = Axis.x*Axis.x*(1.0f - Cos) + Cos;
	Result.a21 = Axis.x*Axis.y*(1.0f - Cos) + Axis.z*Sin;
	Result.a31 = Axis.x*Axis.z*(1.0f - Cos) - Axis.y*Sin;
	Result.a41 = 0.0f;

	Result.a12 = Axis.x*Axis.y*(1.0f - Cos) - Axis.z*Sin;
	Result.a22 = Axis.y*Axis.y*(1.0f - Cos) + Cos;
	Result.a32 = Axis.y*Axis.z*(1.0f - Cos) + Axis.x*Sin;
	Result.a42 = 0.0f;

	Result.a13 = Axis.x*Axis.z*(1.0f - Cos) + Axis.y*Sin;
	Result.a23 = Axis.y*Axis.z*(1.0f - Cos) - Axis.x*Sin;
	Result.a33 = Axis.z*Axis.z*(1.0f - Cos) + Cos;
	Result.a43 = 0.0f;

	Result.a14 = 0.0f;
	Result.a24 = 0.0f;
	Result.a34 = 0.0f;
	Result.a44 = 1.0f;

	return(Result);
}

internal mat4
LookAt(v3 From, v3 Target, v3 UpAxis = V3(0.0f, 1.0f, 0.0f))
{
	v3 Forward = Normalize(From - Target);
	v3 Right = Normalize(Cross(UpAxis, Forward));
	v3 Up = Normalize(Cross(Forward, Right));

	mat4 Result;

	Result.a11 = Right.x;
	Result.a12 = Right.y;
	Result.a13 = Right.z;

	Result.a21 = Up.x;
	Result.a22 = Up.y;
	Result.a23 = Up.z;

	Result.a31 = Forward.x;
	Result.a32 = Forward.y;
	Result.a33 = Forward.z;

	Result.a14 = -Dot(From, Right);
	Result.a24 = -Dot(From, Up);
	Result.a34 = -Dot(From, Forward);
	
	Result.a41 = 0.0f;
	Result.a42 = 0.0f;
	Result.a43 = 0.0f;
	Result.a44 = 1.0f;

	return(Result);
}

internal mat4 
RotationMatrixFromDirectionVector(v3 Direction)
{
	v3 UpAxis = V3(0.0f, 1.0f, 0.0f);
	v3 Forward = Normalize(-Direction);
	v3 Right = Normalize(Cross(UpAxis, Forward));
	v3 Up = Normalize(Cross(Forward, Right));

	mat4 Result;

	Result.a11 = Right.x;
	Result.a12 = Right.y;
	Result.a13 = Right.z;

	Result.a21 = Up.x;
	Result.a22 = Up.y;
	Result.a23 = Up.z;

	Result.a31 = Forward.x;
	Result.a32 = Forward.y;
	Result.a33 = Forward.z;

	Result.a14 = 0.0f;
	Result.a24 = 0.0f;
	Result.a34 = 0.0f;

	Result.a41 = 0.0f;
	Result.a42 = 0.0f;
	Result.a43 = 0.0f;
	Result.a44 = 1.0f;

	return(Result);
}

internal mat4
Ortho(real32 Bottom, real32 Top, real32 Left, real32 Right, real32 Near, real32 Far)
{
	mat4 Result = {};

	Result.a11 = 2.0f / (Right - Left);
	Result.a22 = 2.0f / (Top - Bottom);
	Result.a33 = -2.0f / (Far - Near);
	Result.a14 = -(Right + Left) / (Right - Left);
	Result.a24 = -(Top + Bottom) / (Top - Bottom);
	Result.a34 = -(Far + Near) / (Far - Near);
	Result.a44 = 1.0f;

	return(Result);
}

internal mat4
Perspective(real32 FoV, real32 AspectRatio, real32 Near, real32 Far)
{
	real32 Scale = tanf(Radians(FoV) * 0.5f) * Near;
	real32 Right = AspectRatio * Scale;
	real32 Left = -Right;
	real32 Top = Scale;
	real32 Bottom = -Top;

	mat4 Result = {};

	Result.a11 = 2.0f * Near / (Right - Left);
	Result.a22 = 2.0f * Near / (Top - Bottom);
	Result.a13 = (Right + Left) / (Right - Left);
	Result.a23 = (Top + Bottom) / (Top - Bottom);
	Result.a33 = -(Far + Near) / (Far - Near);
	Result.a43 = -1.0f;
	Result.a34 = -(2.0f * Far*Near) / (Far - Near);

	return(Result);
}