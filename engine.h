#pragma once

#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "irrKlang.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include "simplexnoise.h"
#include <Windows.h>
#include <intrin.h>
#include <vector>
#include <map>
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

#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

#define global_variable static
#define local_persist static
#define internal static

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)

#define Minimum(A, B) ((A < B) ? A : B)
#define Maximum(A, B) ((A > B) ? A : B)

#define M_PI 3.14159265359f

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
			bool32 MoveUp;
		};
	};
};

struct stack_allocator
{
	memory_size Size;
	uint8 *Base;
	memory_size Used;
};

struct temporary_memory
{
	stack_allocator *Allocator;
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

#define PushStructSynchronized(Allocator, type, Semaphore) (type *)PushSizeSynchronized(Allocator, sizeof(type), Semaphore)
#define PushArraySynchronized(Allocator, Count, type, Semaphore) (type *)PushSizeSynchronized(Allocator, (Count)*sizeof(type), Semaphore)
inline void * 
PushSizeSynchronized(stack_allocator *Allocator, memory_size Size, HANDLE Semaphore)
{
	WaitForSingleObjectEx(Semaphore, INFINITE, false);

	Assert((Allocator->Used + Size) < Allocator->Size);
	void *Result = Allocator->Base + Allocator->Used;
	Allocator->Used += Size;

	ReleaseSemaphore(Semaphore, 1, 0);

	return(Result);
}

inline temporary_memory
BeginTemporaryMemory(stack_allocator *Allocator)
{
	temporary_memory Result;

	Result.Allocator = Allocator;
	Result.Used = Allocator->Used;

	return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
	stack_allocator *Allocator = TempMem.Allocator;
	Assert(Allocator->Used >= TempMem.Used);
	Allocator->Used = TempMem.Used;
}

inline void
SubMemory(stack_allocator *Result, stack_allocator *Allocator, memory_size Size)
{
	Result->Size = Size;
	Result->Base = (uint8 *)PushSize(Allocator, Size);
	Result->Used = 0;
}

#include "job_system.hpp"
#include <math.h>
#include "vec.hpp"
#include "maths.hpp"
#include "mat.hpp"
#include "utils.hpp"
#include "shader.hpp"
#include "block_particle_generator.hpp"

struct hero_control
{
	v3 ddP;
	real32 dY;

	bool32 Attack;
	bool32 Fireball;

	real32 Rot;
};

#include "sound_manager.hpp"
#include "qubicle_binary_loader.hpp"
#include "world.hpp"

inline sim_entity_collision_volume_group *
MakeNullCollision(stack_allocator *Allocator)
{
	sim_entity_collision_volume_group *Group = PushStruct(Allocator, sim_entity_collision_volume_group);
	Group->VolumeCount = 0;
	Group->Volumes = 0;

	return(Group);
}

inline sim_entity_collision_volume_group *
MakeSimpleCollision(stack_allocator *Allocator, v3 Dim, v3 Offset)
{
	sim_entity_collision_volume_group *Group = PushStruct(Allocator, sim_entity_collision_volume_group);
	Group->VolumeCount = 1;
	Group->Volumes = PushStruct(Allocator, sim_entity_collision_volume);
	Group->Volumes[0].Dim = Dim;
	Group->Volumes[0].OffsetP = Offset;

	return(Group);
}

struct character
{
	GLuint TextureID;
	v2 Size;
	v2 Bearing;
	uint32 AdvanceToNext;
};

struct text_renderer
{
	GLuint VAO, VBO;
	std::map<GLchar, character> Characters;
};

inline void
InitializeTextRenderer(text_renderer *TextRenderer)
{
	FT_Library FreeType;
	if (FT_Init_FreeType(&FreeType))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
	}

	FT_Face Font;
	if (FT_New_Face(FreeType, "data/fonts/arial.ttf", 0, &Font))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;	}

	FT_Set_Pixel_Sizes(Font, 0, 48);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (uint8 C = 0; C < 128; C++)
	{
		if (FT_Load_Char(Font, C, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << (char)C << std::endl;
			continue;		}

		GLuint Texture;
		glGenTextures(1, &Texture);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Font->glyph->bitmap.width, Font->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, Font->glyph->bitmap.buffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		character Character = {Texture, V2((real32)Font->glyph->bitmap.width, (real32)Font->glyph->bitmap.rows),
							   V2((real32)Font->glyph->bitmap_left, (real32)Font->glyph->bitmap_top), (uint32)Font->glyph->advance.x};
		TextRenderer->Characters.insert(std::pair<GLchar, character>(C, Character));
	}

	FT_Done_Face(Font);
	FT_Done_FreeType(FreeType);

	glGenVertexArrays(1, &TextRenderer->VAO);
	glGenBuffers(1, &TextRenderer->VBO);
	glBindVertexArray(TextRenderer->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, TextRenderer->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(real32) * 4 * 4, 0, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(real32), 0);
	glBindVertexArray(0);
}

internal void
RenderText(text_renderer *TextRenderer, shader &Shader, char *String, real32 X, real32 Y, real32 Scale, v3 Color)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Shader.Enable();
	glUniform3fv(glGetUniformLocation(Shader.ID, "TextColor"), 1, &Color.x);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(TextRenderer->VAO);

	for (char *C = String; *C != '\0'; C++)
	{
		character Ch = TextRenderer->Characters[*C];

		real32 XPos = X + Ch.Bearing.x * Scale;
		real32 YPos = Y - (Ch.Size.y - Ch.Bearing.y) * Scale;

		real32 W = Ch.Size.x * Scale;
		real32 H = Ch.Size.y * Scale;

		real32 Vertices[4][4] = 
			{
				{ XPos,  YPos + H, 0.0f, 0.0f },
				{ XPos,  YPos, 0.0f, 1.0f },
				{ XPos + W,  YPos + H, 1.0f, 0.0f },
				{ XPos + W,  YPos, 1.0f, 1.0f}
			};

		glBindTexture(GL_TEXTURE_2D, Ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, TextRenderer->VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		X += (Ch.AdvanceToNext >> 6) * Scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}

internal void 
RenderTextNumber(text_renderer *TextRenderer, shader &Shader, uint32 Num, real32 X, real32 Y, real32 Scale, v3 Color)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Shader.Enable();
	glUniform3fv(glGetUniformLocation(Shader.ID, "TextColor"), 1, &Color.x);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(TextRenderer->VAO);

	uint32 Length = 0;
	uint32 Temp = Num;
	while (Temp > 0)
	{
		Length++;
		Temp /= 10;
	}

	for(uint32 I = 0; I < Length; I++)
	{
		uint32 Digit = Num;
		while (Digit > 10)
		{
			Digit /= 10;
		}
		Num = Num % (10 * (Length - I));
	
		character Ch = TextRenderer->Characters[Digit + 48];

		real32 XPos = X + Ch.Bearing.x * Scale;
		real32 YPos = Y - (Ch.Size.y - Ch.Bearing.y) * Scale;

		real32 W = Ch.Size.x * Scale;
		real32 H = Ch.Size.y * Scale;

		real32 Vertices[4][4] =
		{
			{ XPos,  YPos + H, 0.0f, 0.0f },
			{ XPos,  YPos, 0.0f, 1.0f },
			{ XPos + W,  YPos + H, 1.0f, 0.0f },
			{ XPos + W,  YPos, 1.0f, 1.0f }
		};

		glBindTexture(GL_TEXTURE_2D, Ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, TextRenderer->VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		X += (Ch.AdvanceToNext >> 6) * Scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}

struct camera
{
	real32 DistanceFromHero;
	real32 Pitch;
	real32 Head;

	real32 RotSensetivity;

	v3 OffsetFromHero;
};

inline void
UpdateCameraOffsetFromHero(camera *Camera)
{
	real32 VerticalDistance = Camera->DistanceFromHero * sinf(-Camera->Pitch / 180.0f * M_PI);
	real32 HorizontalDistance = Camera->DistanceFromHero * cosf(-Camera->Pitch / 180.0f * M_PI);
	real32 XOffsetFromHero = HorizontalDistance * sinf(-Camera->Head / 180.0f * M_PI);
	real32 ZOffsetFromHero = HorizontalDistance * cosf(-Camera->Head / 180.0f * M_PI);
	Camera->OffsetFromHero = V3(XOffsetFromHero, VerticalDistance, ZOffsetFromHero);
}

struct game
{
	game_input Input;
	sound_system Sound;

	job_system_queue *JobSystem;

	HANDLE WorldAllocatorSemaphore;
	stack_allocator WorldAllocator;
	stack_allocator TranAllocator;

	world World;
	graphics_assets *Assets;

	camera Camera;	

	low_entity *Hero;
	hero_control HeroControl;

	block_particle_generator BlockParticleGenerator;

	text_renderer TextRenderer;

	sim_entity_collision_volume_group *NullCollision;
	sim_entity_collision_volume_group *TreeCollision;
};

#include "sim_region.hpp"

struct add_low_entity_result
{
	low_entity *LowEntity;
	uint32 StorageIndex;
};
internal add_low_entity_result
AddLowEntity(game *Game, entity_type Type, world_position P, v3 Dim)
{
	add_low_entity_result Result;
	world *World = &Game->World;
	Assert(World->LowEntityCount < ArrayCount(World->LowEntities));
	uint32 EntityIndex = World->LowEntityCount++;

	low_entity *LowEntity = World->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Sim.StorageIndex = EntityIndex;
	LowEntity->Sim.Type = Type;
	LowEntity->Sim.Dim = Dim;
	LowEntity->Sim.Collision = Game->NullCollision;
	LowEntity->P = InvalidPosition();

	ChangeEntityLocation(World, &Game->WorldAllocator, EntityIndex, LowEntity, P);

	Result.LowEntity = LowEntity;
	Result.StorageIndex = EntityIndex;

	return(Result);
}

internal void
AddTree(game *Game, world_position P, v3 Dim)
{
	add_low_entity_result Entity = AddLowEntity(Game, EntityType_Tree, P, Dim);
	Entity.LowEntity->Sim.Collides = true;
}

internal uint32
AddFireball(game *Game)
{
	add_low_entity_result Entity = AddLowEntity(Game, EntityType_Fireball, InvalidPosition(), V3(0.3f, 0.3f, 0.3f));
	Entity.LowEntity->Sim.Collides = true;
	Entity.LowEntity->Sim.Moveable = true;
	Entity.LowEntity->Sim.NonSpatial = true;

	return(Entity.StorageIndex);
}

internal low_entity *
AddHero(game *Game, world_position P, v3 Dim)
{
	add_low_entity_result Entity = AddLowEntity(Game, EntityType_Hero, P, Dim);

	Entity.LowEntity->Sim.Moveable = true;
	Entity.LowEntity->Sim.Collides = true;
	Entity.LowEntity->Sim.Fireball.LowIndex = AddFireball(Game);

	return(Entity.LowEntity);
}