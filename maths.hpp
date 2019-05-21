#pragma once

struct rect3
{
	v3 Min;
	v3 Max;
};

inline rect3
RectMinMax(v3 Min, v3 Max)
{
	rect3 Result;

	Result.Min = Min;
	Result.Max = Max;

	return(Result);
}

inline rect3
RectCenterHalfDim(v3 Center, v3 HalfDim)
{
	rect3 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return(Result);
}

inline rect3
AddRadiusTo(rect3 A, v3 Radius)
{
	rect3 Result;

	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;

	return(Result);
}

inline rect3
Offset(rect3 A, v3 Offset)
{
	rect3 Result;

	Result.Min = A.Min + Offset;
	Result.Max = A.Max + Offset;

	return(Result);
}

inline rect3
RectMinDim(v3 Min, v3 Dim)
{
	rect3 Result;

	Result.Min = Min;
	Result.Max = Min + Dim;

	return(Result);
}

inline rect3
RectCenterDim(v3 Center, v3 Dim)
{
	rect3 Result = RectCenterHalfDim(Center, 0.5f*Dim);

	return(Result);
}

inline bool32
IsInRect(rect3 Rect, v3 Point)
{
	bool32 Result = ((Point.x >= Rect.Min.x) &&
					 (Point.y >= Rect.Min.y) &&
					 (Point.z >= Rect.Min.z) &&
					 (Point.x < Rect.Max.x) &&
					 (Point.y < Rect.Max.y) &&
			         (Point.z < Rect.Max.z));

	return(Result);
}

inline bool32
RectIntersect(rect3 A, rect3 B)
{
	bool32 Result = !((B.Max.x <= A.Min.x) ||
					 (B.Max.y <= A.Min.y) || 
					 (B.Max.z <= A.Min.z) || 
					 (B.Min.x >= A.Max.x) || 
					 (B.Min.y >= A.Max.y) || 
					 (B.Min.z >= A.Max.z));

	return(Result);
}