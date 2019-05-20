#pragma once

#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "irrKlang.h"
#include <iostream>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;

typedef float real32;
typedef double real64;

typedef size_t memory_size;

#define Assert(expr) if(!(expr)) { *(int *)0 = 0; }

#define global_variable static
#define internal static

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)

#define PI 3.14159265359f

struct stack_allocator
{
	memory_size Size;
	uint8 *Base;
	memory_size Used;
};

inline void 
InitializeStackAllocator(stack_allocator *Allocator, memory_size Size, void *Base)
{
	Allocator->Size = Size;
	Allocator->Base = (uint8 *)Base;
	Allocator->Used = 0;
}

#define PushStruct(Allocator, type) (type *)PushSize(Allocator, sizeof(type))
#define PushArray(Allocator, Count, type) (type *)PushSize(Allocator, (Count)*sizeof(type))
inline void * 
PushSize(stack_allocator *Allocator, memory_size Size)
{
	Assert((Allocator->Used + Size) < Allocator->Size);
	void *Result = Allocator->Base + Allocator->Used;
	Allocator->Used += Size;

	return(Result);
}

#include <math.h>
#include "vec.hpp"
#include "mat.hpp"
#include "shader.hpp"
#include "sound_manager.hpp"

struct game_input
{
	int32 MouseX, MouseY;
	bool32 MouseLeft, MouseRight;
	union
	{
		bool32 Buttons[5];
		struct
		{
			bool32 MoveForward;
			bool32 MoveBack;
			bool32 MoveLeft;
			bool32 MoveRight;
		};
	};
};

struct camera
{
	v3 Position;
	v3 Front;

	real32 Pitch;
	real32 Head;

	real32 MoveSpeed;
	real32 RotSensetivity;
};

struct game
{
	game_input Input;
	sound_system Sound;

	stack_allocator WorldAllocator;

	camera Camera;
};