#pragma once

#define CHUNK_DIM 16
struct world_position
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	v3 Offset;
};

enum block_type
{
	BlockType_None,

	BlockType_Stone,
	BlockType_Grass,
	BlockType_Dirt,
	BlockType_Snow,

	BlockType_Count
};

enum biome_type
{
	BiomeType_GrassLand,

	BiomeType_Count
};

struct biome_height_boundary
{
	real32 HeightBoundary;
	real32 r1, g1, b1;
	real32 r2, g2, b2;
	block_type BlockType;
};

struct biome
{
	std::vector<biome_height_boundary> *BiomeHeightBoundaries;
};

#define GetVoxel(Chunk, X, Y, Z) Chunk[(X) + (Y)*CHUNK_DIM + (Z)*CHUNK_DIM*CHUNK_DIM]
struct block
{
	bool32 Active; 
};
struct block_vertex
{
	v3 Pos;
	v3 Normal;
	v3 Color;
	real32 Occlusion;
};

struct occlusion
{
	real32 Left, Right, Top, Bottom, Front, Back;
};

// TODO(georgy): Decide how to free chunks that haven't been used for a while;
//				 From chunk we should free Blocks, VertexBuffer, VBO & VAO;
//				 Maintain all loaded chunks and then once in some amount of time iterate over them and free that we don't need??
//               Blocks can be freed using FirstFreeBlocks pointer;
//               The contents of VertexBuffer can be freed using trick vector<block_vertex>().swap(VertexBuffer);
//			     To free VBO & VAO use glDeleteBuffer and so on;
//               Don't forget to unset IsLoaded!
struct world_chunk
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	block *Blocks;
	uint32 *Colors;
	block_type *BlockTypes;
	occlusion *Occlusions;
	std::vector<block_vertex> *VertexBuffer;

	bool32 IsSetup;
	bool32 IsLoaded;
	bool32 IsModified;

	GLuint VAO, VBO;

	v3 Translation;

	world_chunk *NextChunk;
	world_chunk *NextChunkSetup;
	world_chunk *NextChunkLoad;
};

#define RAY_COUNT 16
#define POINTS_ON_RAY 3

struct occlusion_ray
{
	real32 Left, Right, Top, Bottom, Front, Back;
	v3 Offsets[POINTS_ON_RAY];
};

struct ray_sample
{
	real32 Left, Right, Top, Bottom, Front, Back;
	occlusion_ray Rays[RAY_COUNT];	
};

struct world
{
	real32 ChunkDimInMeters;
	real32 BlockDimInMeters;

	biome Biomes[BiomeType_Count];

	ray_sample RaySample;

	world_chunk *ChunksSetupList;
	world_chunk *ChunksLoadList;
	world_chunk *ChunksRenderList;

	// NOTE(georgy): Must be a power of two!
	world_chunk *ChunkHash[4096];
};

internal void
InitRaySample(world *World)
{
	ray_sample *RaySample = &World->RaySample;

	real32 Inc = M_PI * (3 - sqrtf(5.0f));
	real32 Off = 2.0f / RAY_COUNT;
	for (int K = 0; K < RAY_COUNT; K++)
	{
		real32 Y = K * Off - 1 + (Off / 2);
		real32 R = sqrtf(1 - Y*Y);
		real32 Phi = K * Inc;
		for (int I = 0; I < POINTS_ON_RAY; I++)
		{
			RaySample->Rays[K].Offsets[I] = (I+1)*1.5f*V3(cosf(Phi)*R, Y, sinf(Phi)*R);
		}
	}

	for (int K = 0; K < RAY_COUNT; K++)
	{
		real32 ContributionRightFace = RaySample->Rays[K].Offsets[0].x;
		if (ContributionRightFace > 0.0f)
		{
			RaySample->Rays[K].Right = ContributionRightFace;
			RaySample->Right += ContributionRightFace;
		}
		real32 ContributionLeftFace = RaySample->Rays[K].Offsets[0].x;
		if (ContributionLeftFace < 0.0f)
		{
			RaySample->Rays[K].Left = -ContributionLeftFace;
			RaySample->Left += -ContributionLeftFace;
		}
		real32 ContributionFrontFace = RaySample->Rays[K].Offsets[0].z;
		if (ContributionFrontFace > 0.0f)
		{
			RaySample->Rays[K].Front = ContributionFrontFace;
			RaySample->Front += ContributionFrontFace;
		}
		real32 ContributionBackFace = RaySample->Rays[K].Offsets[0].z;
		if (ContributionBackFace < 0.0f)
		{
			RaySample->Rays[K].Back = -ContributionBackFace;
			RaySample->Back += -ContributionBackFace;
		}
		real32 ContributionTopFace = RaySample->Rays[K].Offsets[0].y;
		if (ContributionTopFace > 0.0f)
		{
			RaySample->Rays[K].Top = ContributionTopFace;
			RaySample->Top += ContributionTopFace;
		}
		real32 ContributionBottomFace = RaySample->Rays[K].Offsets[0].y;
		if (ContributionBottomFace < 0.0f)
		{
			RaySample->Rays[K].Bottom = -ContributionBottomFace;
			RaySample->Bottom += -ContributionBottomFace;
		}
	}
}

internal uint32
GetUInt32Color(real32 r, real32 g, real32 b)
{
	if (r > 1.0f) r = 1.0f;
	if (g > 1.0f) g = 1.0f;
	if (b > 1.0f) b = 1.0f;
	if (r < 0.0f) r = 0.0f;
	if (g < 0.0f) g = 0.0f;
	if (b < 0.0f) b = 0.0f;
	real32 a = 1.0f;

	uint32 Result = (((uint32)(a * 255) << 24) |
					 ((uint32)(b * 255) << 16) |
					 ((uint32)(g * 255) << 8)  |
					 ((uint32)(r * 255)));

	return(Result);
}

internal v3
GetRGBColorFromUInt32(uint32 Color)
{
	v3 Result;

	uint32 r = (Color & 0x000000FF);
	uint32 g = (Color & 0x0000FF00) >> 8;
	uint32 b = (Color & 0x00FF0000) >> 16;

	Result.r = (r / 255.0f);
	Result.g = (g / 255.0f);
	Result.b = (b / 255.0f);

	return(Result);
}

inline biome_height_boundary 
InitBoundary(real32 HeightBoundary, real32 r1, real32 g1, real32 b1, 
									real32 r2, real32 g2, real32 b2, block_type BlockType)
{
	biome_height_boundary Result;
	Result.HeightBoundary = HeightBoundary;
	Result.r1 = r1;
	Result.g1 = g1;
	Result.b1 = b1;
	Result.r2 = r2;
	Result.g2 = g2;
	Result.b2 = b2;
	Result.BlockType = BlockType;

	return(Result);
}

internal void
InitBiomes(world *World)
{
	World->Biomes[BiomeType_GrassLand].BiomeHeightBoundaries = new std::vector<biome_height_boundary>;
	biome_height_boundary Boundary = InitBoundary(-0.5f, 0.4f, 0.4f, 0.4f, 0.25f, 0.3f, 0.27f, BlockType_Stone);
	World->Biomes[BiomeType_GrassLand].BiomeHeightBoundaries->push_back(Boundary);
	Boundary = InitBoundary(0.0f, 0.84f, 0.64f, 0.24f, 0.4f, 0.19f, 0.1f, BlockType_Dirt);
	World->Biomes[BiomeType_GrassLand].BiomeHeightBoundaries->push_back(Boundary);
	Boundary = InitBoundary(0.5f, 0.0f, 0.46f, 0.16f, 0.65f, 0.8f, 0.0f, BlockType_Grass);
	World->Biomes[BiomeType_GrassLand].BiomeHeightBoundaries->push_back(Boundary);
	Boundary = InitBoundary(1.0f, 0.75f, 0.75f, 0.75f, 0.67f, 0.55f, 0.7f, BlockType_Snow);
	World->Biomes[BiomeType_GrassLand].BiomeHeightBoundaries->push_back(Boundary);
}

internal void
GetColorAndBlockType(world *World, real32 HeightNoise, real32 ColorNoise, real32 *r, real32 *g, real32 *b, block_type *BlockType)
{
	// TODO(georgy): Implement more biomes and getting a biome
	// biome Biome = GetBiome(ChunkX, ChunkY, ChunkZ);
	biome *Biome = World->Biomes + BiomeType_GrassLand;

	real32 r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
	real32 r2 = 1.0f, g2 = 1.0f, b2 = 1.0f;

	uint32 BiomeBlocksCount = (uint32)Biome->BiomeHeightBoundaries->size();
	for (uint32 I = 0; I < BiomeBlocksCount; I++)
	{
		if (HeightNoise <= (*Biome->BiomeHeightBoundaries)[I].HeightBoundary)
		{
			r1 = (*Biome->BiomeHeightBoundaries)[I].r1;
			g1 = (*Biome->BiomeHeightBoundaries)[I].g1;
			b1 = (*Biome->BiomeHeightBoundaries)[I].b1;

			r2 = (*Biome->BiomeHeightBoundaries)[I].r2;
			g2 = (*Biome->BiomeHeightBoundaries)[I].g2;
			b2 = (*Biome->BiomeHeightBoundaries)[I].b2;

			*BlockType = (*Biome->BiomeHeightBoundaries)[I].BlockType;

			break;
		}
	}

	*r = r1 + (r2 - r1)*ColorNoise;
	*g = g1 + (g2 - g1)*ColorNoise;
	*b = b1 + (b2 - b1)*ColorNoise;
}

inline v3
Substract(world *World, world_position *A, world_position *B)
{
	v3 dChunk = { (real32)(A->ChunkX - B->ChunkX),
				  (real32)(A->ChunkY - B->ChunkY),
				  (real32)(A->ChunkZ - B->ChunkZ) };

	v3 Result = World->ChunkDimInMeters*dChunk + (A->Offset - B->Offset);

	return(Result);
}

internal void 
LoadChunks(world *World)
{
	for(world_chunk *Chunk = World->ChunksLoadList; Chunk; Chunk = Chunk->NextChunkLoad)
	{
		glGenVertexArrays(1, &Chunk->VAO);
		glGenBuffers(1, &Chunk->VBO);
		glBindVertexArray(Chunk->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, Chunk->VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(block_vertex)*Chunk->VertexBuffer->size(), &(*Chunk->VertexBuffer)[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*3));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*6));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*9));
		glBindVertexArray(0);

		Chunk->IsLoaded = true;
	}
}

internal void
RenderChunks(world *World, GLuint Shader, mat4 *ViewRotation)
{
	for(world_chunk *Chunk = World->ChunksRenderList; Chunk; Chunk = Chunk->NextChunk)
	{
		mat4 TranslationMatrix = Translation(Chunk->Translation);
		mat4 Matrix = *ViewRotation * TranslationMatrix;
		glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, Matrix.Elements);

		glBindVertexArray(Chunk->VAO);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)Chunk->VertexBuffer->size());
	}
}

internal v3
GetCoordinatesFromWorldPos(world *World, world_position *WorldPos)
{
	v3 Result;

	Result.x = World->ChunkDimInMeters*WorldPos->ChunkX + WorldPos->Offset.x;
	Result.y = World->ChunkDimInMeters*WorldPos->ChunkY + WorldPos->Offset.y;
	Result.z = World->ChunkDimInMeters*WorldPos->ChunkZ + WorldPos->Offset.z;

	return(Result);
}

inline void
RecanonicalizeCoords(world *World, world_position *Pos)
{
	int32 NewChunkXOffset = (int32)floorf(Pos->Offset.x / World->ChunkDimInMeters);
	Pos->ChunkX += NewChunkXOffset;
	Pos->Offset.x -= NewChunkXOffset*World->ChunkDimInMeters;

	int32 NewChunkYOffset = (int32)floorf(Pos->Offset.y / World->ChunkDimInMeters);
	Pos->ChunkY += NewChunkYOffset;
	Pos->Offset.y -= NewChunkYOffset*World->ChunkDimInMeters;

	int32 NewChunkZOffset = (int32)floorf(Pos->Offset.z / World->ChunkDimInMeters);
	Pos->ChunkZ += NewChunkZOffset;
	Pos->Offset.z -= NewChunkZOffset*World->ChunkDimInMeters;
}

inline world_position
MapIntoChunkSpace(world *World, world_position WorldPos, v3 Offset)
{
	world_position Result = WorldPos;
	
	Result.Offset += Offset;
	RecanonicalizeCoords(World, &Result);

	return(Result);
}

inline void 
AddQuad(std::vector<block_vertex> *VertexBuffer, block_vertex *A, block_vertex *B,
												 block_vertex *C, block_vertex *D)
{
	VertexBuffer->push_back(*A);
	VertexBuffer->push_back(*B);
	VertexBuffer->push_back(*C);
	VertexBuffer->push_back(*C);
	VertexBuffer->push_back(*B);
	VertexBuffer->push_back(*D);
}

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
internal world_chunk * 
GetWorldChunk(world *World, stack_allocator *WorldAllocator, int32 ChunkX, int32 ChunkY, int32 ChunkZ)
{
	Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);

	uint32 HashValue = 19*ChunkX + 3*ChunkY + 7*ChunkZ;
	uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
	Assert(HashSlot < ArrayCount(World->ChunkHash));

	world_chunk **ChunkPtr = World->ChunkHash + HashSlot;
	world_chunk *Chunk = *ChunkPtr;
	while(Chunk)
	{
		if(Chunk->ChunkX == ChunkX &&
		   Chunk->ChunkY == ChunkY &&
		   Chunk->ChunkZ == ChunkZ)
		{
			break;
		}
		
		if(!Chunk->NextChunk)
		{
			Chunk->NextChunk = PushStruct(WorldAllocator, world_chunk);
			Chunk = Chunk->NextChunk;

			Chunk->ChunkX = ChunkX;
			Chunk->ChunkY = ChunkY;
			Chunk->ChunkZ = ChunkZ;

			Chunk->IsLoaded = false;
			Chunk->IsModified = false;

			Chunk->VertexBuffer = new std::vector<block_vertex>;

			Chunk->NextChunk = 0;
			break;
		}

		Chunk = Chunk->NextChunk;
	}

	if(Chunk == 0)
	{
		Chunk = PushStruct(WorldAllocator, world_chunk);
		Chunk->ChunkX = ChunkX;
		Chunk->ChunkY = ChunkY;
		Chunk->ChunkZ = ChunkZ;

		Chunk->IsLoaded = false;
		Chunk->IsModified = false;

		Chunk->VertexBuffer = new std::vector<block_vertex>;

		Chunk->NextChunk = 0;
	
		*ChunkPtr = Chunk;
	}

	return(Chunk);
}

internal void
GenerateChunkVertices(world *World, world_chunk *Chunk, real32 BlockDimInMeters)
{
	block *Blocks = Chunk->Blocks;
	for (uint32 ZI = 0; ZI < CHUNK_DIM; ZI++)
	{
		for (uint32 YI = 0; YI < CHUNK_DIM; YI++)
		{
			for (uint32 XI = 0; XI < CHUNK_DIM; XI++)
			{
				if (GetVoxel(Blocks, XI, YI, ZI).Active)
				{
					real32 X = BlockDimInMeters*((real32)XI);
					real32 Y = BlockDimInMeters*((real32)YI);
					real32 Z = BlockDimInMeters*((real32)ZI);
					block_vertex A, B, C, D;

					v3 Color = GetRGBColorFromUInt32(GetVoxel(Chunk->Colors, XI, YI, ZI));
					A.Color = B.Color = C.Color = D.Color = Color;

					if ((XI == 0) || !GetVoxel(Blocks, XI - 1, YI, ZI).Active)
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X, Y, Z + BlockDimInMeters);
						C.Pos = V3(X, Y + BlockDimInMeters, Z);
						D.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(-1.0f, 0.0f, 0.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Left;
						ThisFaceRayContribution /= World->RaySample.Left;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if ((XI == CHUNK_DIM-1) || !GetVoxel(Blocks, XI + 1, YI, ZI).Active)
					{
						A.Pos = V3(X + BlockDimInMeters, Y, Z);
						B.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						C.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(1.0f, 0.0f, 0.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Right;
						ThisFaceRayContribution /= World->RaySample.Right;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if ((ZI == 0) || !GetVoxel(Blocks, XI, YI, ZI - 1).Active)
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X, Y + BlockDimInMeters, Z);
						C.Pos = V3(X + BlockDimInMeters, Y, Z);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, -1.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Back;
						ThisFaceRayContribution /= World->RaySample.Back;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((ZI == CHUNK_DIM-1) || !GetVoxel(Blocks, XI, YI, ZI+1).Active)
					{
						A.Pos = V3(X, Y, Z + BlockDimInMeters);
						B.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						C.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, 1.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Front;
						ThisFaceRayContribution /= World->RaySample.Front;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((YI == 0) || (!GetVoxel(Blocks, XI, YI-1, ZI).Active))
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X + BlockDimInMeters, Y, Z);
						C.Pos = V3(X, Y, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, -1.0f, 0.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Bottom;
						ThisFaceRayContribution /= World->RaySample.Bottom;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((YI == CHUNK_DIM - 1) || (!GetVoxel(Blocks, XI, YI+1, ZI).Active))
					{
						A.Pos = V3(X, Y + BlockDimInMeters, Z);
						B.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						C.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 1.0f, 0.0f);
						real32 ThisFaceRayContribution = GetVoxel(Chunk->Occlusions, XI, YI, ZI).Top;
						ThisFaceRayContribution /= World->RaySample.Top;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}
				}
			}
		}
	}
}

struct setup_chunk_data
{
	world *World;
	stack_allocator *WorldAllocator;
	world_chunk *Chunk;
	HANDLE WorldAllocatorSemaphore;
};
internal void
SetupChunk(void *Data)
{
	setup_chunk_data *Work = (setup_chunk_data *)Data;
	world *World = Work->World;
	stack_allocator *WorldAllocator = Work->WorldAllocator;
	world_chunk *Chunk = Work->Chunk;
	HANDLE WorldAllocatorSemaphore = Work->WorldAllocatorSemaphore;
	Chunk->Blocks = PushArraySynchronized(WorldAllocator, CHUNK_DIM*CHUNK_DIM*CHUNK_DIM, block, WorldAllocatorSemaphore);
	Chunk->Colors = PushArraySynchronized(WorldAllocator, CHUNK_DIM*CHUNK_DIM*CHUNK_DIM, uint32, WorldAllocatorSemaphore);
	Chunk->BlockTypes = PushArraySynchronized(WorldAllocator, CHUNK_DIM*CHUNK_DIM*CHUNK_DIM, block_type, WorldAllocatorSemaphore);
	Chunk->Occlusions = PushArraySynchronized(WorldAllocator, CHUNK_DIM*CHUNK_DIM*CHUNK_DIM, occlusion, WorldAllocatorSemaphore);
	block *Blocks = Chunk->Blocks;
	real32 BlockDimInMeters = World->BlockDimInMeters;
	for (uint32 Z = 0; Z < CHUNK_DIM; Z++)
	{
		for (uint32 X = 0; X < CHUNK_DIM; X++)
		{	
			real32 Noise = octave_noise_2d(4.0f, 0.3f, 0.0075f,
										   (real32)((Chunk->ChunkX * CHUNK_DIM) + X),
										   (real32)((Chunk->ChunkZ * CHUNK_DIM) + Z));
			real32 Height;
			if (Chunk->ChunkY < 0)
			{
				Height = CHUNK_DIM;
			}
			else
			{
				Height = (Noise + 1.0f) * 0.5f * CHUNK_DIM;
				Assert(Height > 0.0f);
				Assert(Height <= CHUNK_DIM);
			}
			for (uint32 Y = 0; Y < Height; Y++)
			{
				GetVoxel(Blocks, X, Y, Z).Active = true;

				real32 ColorNoise = octave_noise_3d(4.0f, 0.3f, 0.005f, (real32)((Chunk->ChunkX * CHUNK_DIM) + X), 
																		(real32)((Chunk->ChunkY * CHUNK_DIM) + Y), 
																		(real32)((Chunk->ChunkZ * CHUNK_DIM) + Z));
				ColorNoise = (ColorNoise + 1.0f) * 0.5f;

				real32 r, g, b;
				block_type BlockType;
				GetColorAndBlockType(World, Noise, ColorNoise, &r, &g, &b, &BlockType);
				GetVoxel(Chunk->BlockTypes, X, Y, Z) = BlockType;
				GetVoxel(Chunk->Colors, X, Y, Z) = GetUInt32Color(r, g, b);

				for(uint32 RayIndex = 0; RayIndex < RAY_COUNT; RayIndex++)
				{
					bool32 Collided = false;
					for(uint32 OffsetIndex = 0; OffsetIndex < POINTS_ON_RAY; OffsetIndex++)
					{
						uint32 XOff = (uint32)(X + roundf(World->RaySample.Rays[RayIndex].Offsets[OffsetIndex].x));
						uint32 YOff = (uint32)(Y + roundf(World->RaySample.Rays[RayIndex].Offsets[OffsetIndex].y));
						uint32 ZOff = (uint32)(Z + roundf(World->RaySample.Rays[RayIndex].Offsets[OffsetIndex].z));
						if (XOff < 0 || XOff >= CHUNK_DIM ||
							YOff < 0 || YOff >= CHUNK_DIM ||
							ZOff < 0 || ZOff >= CHUNK_DIM)
						{
							break;
						}
						if(GetVoxel(Blocks, XOff, YOff, ZOff).Active)
						{
							Collided = true;
							break;
						}
					}
					if(!Collided)
					{
						GetVoxel(Chunk->Occlusions, X, Y, Z).Left += World->RaySample.Rays[RayIndex].Left;
						GetVoxel(Chunk->Occlusions, X, Y, Z).Right += World->RaySample.Rays[RayIndex].Right;
						GetVoxel(Chunk->Occlusions, X, Y, Z).Top += World->RaySample.Rays[RayIndex].Top;
						GetVoxel(Chunk->Occlusions, X, Y, Z).Bottom += World->RaySample.Rays[RayIndex].Bottom;
						GetVoxel(Chunk->Occlusions, X, Y, Z).Front += World->RaySample.Rays[RayIndex].Front;
						GetVoxel(Chunk->Occlusions, X, Y, Z).Back += World->RaySample.Rays[RayIndex].Back;
					}
				}
			}
		}
	}

	GenerateChunkVertices(World, Chunk, BlockDimInMeters);

	Chunk->IsSetup = true;
}

internal void
SetupChunks(job_system_queue *JobSystem, world *World, stack_allocator *WorldAllocator, stack_allocator *TranAllocator, HANDLE WorldAllocatorSemaphore)
{
	for (world_chunk *Chunk = World->ChunksSetupList; Chunk; Chunk = Chunk->NextChunkSetup)
	{
		setup_chunk_data *SetupChunkData = PushStruct(TranAllocator, setup_chunk_data);
		SetupChunkData->World = World;
		SetupChunkData->WorldAllocator = WorldAllocator;
		SetupChunkData->Chunk = Chunk;
		SetupChunkData->WorldAllocatorSemaphore = WorldAllocatorSemaphore;
		AddEntryToJobSystem(JobSystem, SetupChunk, SetupChunkData);
	}

	CompleteAllWork(JobSystem);
}

#if 0
internal void
UpdateChunk(world *World, world_chunk *Chunk)
{
	Chunk->VertexBuffer->resize(0);
	
	block *Blocks = Chunk->Blocks;
	real32 BlockDimInMeters = World->BlockDimInMeters;

	GenerateChunkVertices(World, Chunk, BlockDimInMeters);

	glBindVertexArray(Chunk->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_vertex)*Chunk->VertexBuffer->size(), &(*Chunk->VertexBuffer)[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*6));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*9));
	glBindVertexArray(0);
}
#endif