#pragma once

#define CHUNK_DIM 16
struct world_position
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	v3 Offset;
};

#define INVALID_POSITION INT32_MAX
inline world_position
InvalidPosition()
{
	world_position Result = {};
	Result.ChunkX = INVALID_POSITION;
	return(Result);
}

inline bool32 
IsValid(world_position P)
{
	bool32 Result = P.ChunkX != INVALID_POSITION;
	return(Result);
}

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

struct world_entity_block
{
	uint32 LowEntityCount;
	uint32 LowEntityIndex[16];
	world_entity_block *Next;
};

struct chunk_data
{
	block Blocks[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];
	uint32 Colors[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];
	block_type BlockTypes[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];
	occlusion Occlusions[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

	chunk_data *Next;
};
struct world_chunk
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;
	
#if 0
	block *Blocks;
	uint32 *Colors;
	block_type *BlockTypes;
	occlusion *Occlusions;
#else
	chunk_data *ChunkData;
#endif
	std::vector<block_vertex> *VertexBuffer;

	bool32 IsSetup;
	bool32 IsLoaded;
	bool32 IsModified;
	bool32 IsNotEmpty;

	GLuint VAO, VBO;

	v3 Translation;
	real32 LengthSquaredToOrigin;

	world_entity_block FirstBlock;

	world_chunk *NextChunk;
	world_chunk *NextChunkSetup;
	world_chunk *NextChunkLoad;
	world_chunk *NextChunkUpdate;
	
	world_chunk *NextChunkUsed;
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

struct sim_entity;
struct entity_reference
{
	uint32 LowIndex;
	sim_entity *SimPtr;
};

enum entity_type
{
	EntityType_Null,

	EntityType_Chunk, // NOTE(georgy): We can't create chunk entities, it is needed for collision detection, if we don't want make chunk an entity
	EntityType_Hero,
	EntityType_Tree,
	EntityType_Fireball,

	EntityType_Count
};

struct sim_entity_collision_volume
{
	v3 OffsetP;
	v3 Dim;
};

struct sim_entity_collision_volume_group
{
	uint32 VolumeCount;
	sim_entity_collision_volume *Volumes;
};

struct sim_entity
{
	uint32 StorageIndex;
	entity_type Type;

	v3 Dim;
	sim_entity_collision_volume_group *Collision;

	entity_reference Fireball;

	bool32 NonSpatial;
	bool32 Collides;
	bool32 Moveable;
	bool32 Updatable;
	bool32 OnGround;

	real32 DistanceLimit;

	v3 P;
	v3 dP;

	v3 FacingDir;
};

struct low_entity
{
	world_position P;
	sim_entity Sim;
};

struct world
{
	real32 ChunkDimInMeters;
	real32 BlockDimInMeters;

	biome Biomes[BiomeType_Count];

	ray_sample RaySample;

	world_chunk *ChunksSetupList;
	world_chunk *ChunksLoadList;
	world_chunk *ChunksUpdateList;
	world_chunk *ChunksRenderList;

	// NOTE(georgy): Must be a power of two!
	world_chunk *ChunkHash[4096];
	world_chunk *RecentlyUsedChunksList;

	uint32 LowEntityCount;
	low_entity LowEntities[10000];

	world_entity_block *FirstFreeEntityBlock;
	
	chunk_data *FirstFreeChunkData;
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

inline void 
InitializeWorld(world *World)
{
	InitRaySample(World);
	World->ChunkDimInMeters = CHUNK_DIM / 2.0f;
	//World->ChunkDimInMeters = CHUNK_DIM;
	World->BlockDimInMeters = World->ChunkDimInMeters / CHUNK_DIM;
	InitBiomes(World);
}

inline bool32
AreInTheSameChunk(world *World, world_position *A, world_position *B)
{
	real32 Epsilon = 0.001f;
	Assert(A->Offset.x < World->ChunkDimInMeters + Epsilon);
	Assert(A->Offset.y < World->ChunkDimInMeters + Epsilon);
	Assert(A->Offset.z < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.x < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.y < World->ChunkDimInMeters + Epsilon);
	Assert(B->Offset.z < World->ChunkDimInMeters + Epsilon);

	bool32 Result = ((A->ChunkX == B->ChunkX) &&
					 (A->ChunkY == B->ChunkY) &&
					 (A->ChunkZ == B->ChunkZ));

	return(Result);
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

internal bool32
FrustumCulling(mat4 MVP, real32 AABBMin, real32 AABBMax)
{
	v4 BoxClipSpace[8];

	real32 MinX, MinY, MinZ;
	real32 MaxX, MaxY, MaxZ;
	MinX = MinY = MinZ = AABBMin;
	MaxX = MaxY = MaxZ = AABBMax;

	real32 Mat00MinX = MVP.Elements[0 + 0*4] * MinX;
	real32 Mat00MaxX = MVP.Elements[0 + 0*4] * MaxX;
	real32 Mat01MinY = MVP.Elements[0 + 1*4] * MinY;
	real32 Mat01MaxY = MVP.Elements[0 + 1*4] * MaxY;
	real32 Mat02MinZW = MVP.Elements[0 + 2*4] * MinZ + MVP.Elements[0 + 3*4];
	real32 Mat02MaxZW = MVP.Elements[0 + 2*4] * MaxZ + MVP.Elements[0 + 3*4];

	real32 Mat10MinX = MVP.Elements[1 + 0*4] * MinX;
	real32 Mat10MaxX = MVP.Elements[1 + 0*4] * MaxX;
	real32 Mat11MinY = MVP.Elements[1 + 1*4] * MinY;
	real32 Mat11MaxY = MVP.Elements[1 + 1*4] * MaxY;
	real32 Mat12MinZW = MVP.Elements[1 + 2*4] * MinZ + MVP.Elements[1 + 3*4];
	real32 Mat12MaxZW = MVP.Elements[1 + 2*4] * MaxZ + MVP.Elements[1 + 3*4];

	real32 Mat20MinX = MVP.Elements[2 + 0*4] * MinX;
	real32 Mat20MaxX = MVP.Elements[2 + 0*4] * MaxX;
	real32 Mat21MinY = MVP.Elements[2 + 1*4] * MinY;
	real32 Mat21MaxY = MVP.Elements[2 + 1*4] * MaxY;
	real32 Mat22MinZW = MVP.Elements[2 + 2*4] * MinZ + MVP.Elements[2 + 3*4];
	real32 Mat22MaxZW = MVP.Elements[2 + 2*4] * MaxZ + MVP.Elements[2 + 3*4];

	real32 Mat30MinX = MVP.Elements[3 + 0*4] * MinX;
	real32 Mat30MaxX = MVP.Elements[3 + 0*4] * MaxX;
	real32 Mat31MinY = MVP.Elements[3 + 1*4] * MinY;
	real32 Mat31MaxY = MVP.Elements[3 + 1*4] * MaxY;
	real32 Mat32MinZW = MVP.Elements[3 + 2*4] * MinZ + MVP.Elements[3 + 3*4];
	real32 Mat32MaxZW = MVP.Elements[3 + 2*4] * MaxZ + MVP.Elements[3 + 3*4];

	BoxClipSpace[0].x = Mat00MinX + Mat01MinY + Mat02MinZW; 
	BoxClipSpace[0].y = Mat10MinX + Mat11MinY + Mat12MinZW; 
	BoxClipSpace[0].z = Mat20MinX + Mat21MinY + Mat22MinZW; 
	BoxClipSpace[0].w = Mat30MinX + Mat31MinY + Mat32MinZW; 
	
	BoxClipSpace[1].x = Mat00MaxX + Mat01MinY + Mat02MinZW; 
	BoxClipSpace[1].y = Mat10MaxX + Mat11MinY + Mat12MinZW; 
	BoxClipSpace[1].z = Mat20MaxX + Mat21MinY + Mat22MinZW; 
	BoxClipSpace[1].w = Mat30MaxX + Mat31MinY + Mat32MinZW;

	BoxClipSpace[2].x = Mat00MinX + Mat01MaxY + Mat02MinZW; 
	BoxClipSpace[2].y = Mat10MinX + Mat11MaxY + Mat12MinZW; 
	BoxClipSpace[2].z = Mat20MinX + Mat21MaxY + Mat22MinZW; 
	BoxClipSpace[2].w = Mat30MinX + Mat31MaxY + Mat32MinZW;

	BoxClipSpace[3].x = Mat00MaxX + Mat01MaxY + Mat02MinZW; 
	BoxClipSpace[3].y = Mat10MaxX + Mat11MaxY + Mat12MinZW; 
	BoxClipSpace[3].z = Mat20MaxX + Mat21MaxY + Mat22MinZW; 
	BoxClipSpace[3].w = Mat30MaxX + Mat31MaxY + Mat32MinZW;

	BoxClipSpace[4].x = Mat00MinX + Mat01MinY + Mat02MaxZW; 
	BoxClipSpace[4].y = Mat10MinX + Mat11MinY + Mat12MaxZW; 
	BoxClipSpace[4].z = Mat20MinX + Mat21MinY + Mat22MaxZW; 
	BoxClipSpace[4].w = Mat30MinX + Mat31MinY + Mat32MaxZW;

	BoxClipSpace[5].x = Mat00MaxX + Mat01MinY + Mat02MaxZW; 
	BoxClipSpace[5].y = Mat10MaxX + Mat11MinY + Mat12MaxZW; 
	BoxClipSpace[5].z = Mat20MaxX + Mat21MinY + Mat22MaxZW; 
	BoxClipSpace[5].w = Mat30MaxX + Mat31MinY + Mat32MaxZW;

	BoxClipSpace[6].x = Mat00MinX + Mat01MaxY + Mat02MaxZW; 
	BoxClipSpace[6].y = Mat10MinX + Mat11MaxY + Mat12MaxZW; 
	BoxClipSpace[6].z = Mat20MinX + Mat21MaxY + Mat22MaxZW; 
	BoxClipSpace[6].w = Mat30MinX + Mat31MaxY + Mat32MaxZW;

	BoxClipSpace[7].x = Mat00MaxX + Mat01MaxY + Mat02MaxZW; 
	BoxClipSpace[7].y = Mat10MaxX + Mat11MaxY + Mat12MaxZW; 
	BoxClipSpace[7].z = Mat20MaxX + Mat21MaxY + Mat22MaxZW; 
	BoxClipSpace[7].w = Mat30MaxX + Mat31MaxY + Mat32MaxZW;

	for(uint32 J = 0; J < 3; J++)
	{
		uint32 In = 0;
		for(uint32 I = 0; (I < ArrayCount(BoxClipSpace)); I++)
		{
			if(BoxClipSpace[I].E[J] > -BoxClipSpace[I].w)
			{
				In++;
				break;
			}
		}
		if(!In)
		{
			return(false);
		}
	}

	for(uint32 J = 0; J < 3; J++)
	{
		uint32 In = 0;
		for(uint32 I = 0; (I < ArrayCount(BoxClipSpace)); I++)
		{
			if(BoxClipSpace[I].E[J] < BoxClipSpace[I].w)
			{
				In++;
				break;
			}
		}
		if(!In)
		{
			return(false);
		}
	}

	return(true);
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
UnloadChunk(world *World, world_chunk *Chunk) 
{
	Chunk->IsSetup = false;
	Chunk->IsLoaded = false;
	
	chunk_data *ChunkData = Chunk->ChunkData;
	ZeroMemory(ChunkData, sizeof(ChunkData->Blocks) + sizeof(ChunkData->Colors) + sizeof(ChunkData->BlockTypes) + sizeof(ChunkData->Occlusions));
	ChunkData->Next = World->FirstFreeChunkData;
	World->FirstFreeChunkData = ChunkData;
	std::vector<block_vertex>().swap(*Chunk->VertexBuffer);
	glDeleteBuffers(1, &Chunk->VBO);
	glDeleteVertexArrays(1, &Chunk->VAO);
}

internal void
UnloadChunks(world *World, world_position MinChunkP, world_position MaxChunkP)
{
	world_chunk **ChunkPtr = &World->RecentlyUsedChunksList;
	world_chunk *Chunk = *ChunkPtr;
	while(Chunk)
	{
		if(Chunk->ChunkX < (MinChunkP.ChunkX - 1) ||
		   Chunk->ChunkY < (MinChunkP.ChunkY - 1) ||
		   Chunk->ChunkZ < (MinChunkP.ChunkZ - 1) ||
		   Chunk->ChunkX > (MaxChunkP.ChunkX + 1) ||
		   Chunk->ChunkY > (MaxChunkP.ChunkY + 1) ||
		   Chunk->ChunkZ > (MaxChunkP.ChunkZ + 1))
		{
			UnloadChunk(World, Chunk);

			*ChunkPtr = Chunk->NextChunkUsed;
		}
		else
		{
			ChunkPtr = &Chunk->NextChunkUsed;
		}
		Chunk = *ChunkPtr;
	}
}

internal bool32
IsRecentlyUsed(world_chunk *UsedChunks, world_chunk *ChunkToTest)
{
	bool32 Result = false;
	for(world_chunk *Chunk = UsedChunks; Chunk; Chunk = Chunk->NextChunkUsed)
	{
		if(Chunk->ChunkX == ChunkToTest->ChunkX && 
		   Chunk->ChunkY == ChunkToTest->ChunkY &&
		   Chunk->ChunkZ == ChunkToTest->ChunkZ)
		{
			Result = true;
			break;
		}
	}

	return(Result);
}

internal world_chunk *
SortedMerge(world_chunk *A, world_chunk *B)
{
	world_chunk Temp;

	world_chunk *Tail = &Temp;
	Temp.NextChunk = 0;
	while(1)
	{
		if(A == 0)
		{
			Tail->NextChunk = B;
			break;
		}
		else if(B == 0)
		{
			Tail->NextChunk = A;
			break;
		}
		if(A->LengthSquaredToOrigin <= B->LengthSquaredToOrigin)
		{
			world_chunk *NewNode = A;
			A = A->NextChunk;
			NewNode->NextChunk = Tail->NextChunk; 
			Tail->NextChunk = NewNode;
		}
		else
		{
			world_chunk *NewNode = B;
			B = B->NextChunk;
			NewNode->NextChunk = Tail->NextChunk; 
			Tail->NextChunk = NewNode;
		}

		Tail = Tail->NextChunk;
	}

	return(Temp.NextChunk);
}

internal void
SplitLinkedList(world_chunk *Source, world_chunk **A, world_chunk **B)
{
	world_chunk *Fast, *Slow;
	Slow = Source;
	Fast = Source->NextChunk;

	while(Fast != 0)
	{
		Fast = Fast->NextChunk;
		if(Fast != 0)
		{
			Slow = Slow->NextChunk;
			Fast = Fast->NextChunk;
		}
	}

	*A = Source;
	*B = Slow->NextChunk;
	Slow->NextChunk = 0;
}

internal void
MergeSort(world_chunk **Node)
{
	world_chunk *Head = *Node;
	world_chunk *A;
	world_chunk *B;

	if((Head == 0) || (Head->NextChunk == 0))
	{
		return;
	}

	SplitLinkedList(Head, &A, &B);

	MergeSort(&A);
	MergeSort(&B);

	*Node = SortedMerge(A, B);
}

internal void
RenderChunks(world *World, GLuint Shader, mat4 *ViewRotation, mat4 *Projection, v3 CameraOffsetFromHero)
{
	MergeSort(&World->ChunksRenderList);
	for(world_chunk *Chunk = World->ChunksRenderList; Chunk; Chunk = Chunk->NextChunk)
	{
		mat4 ModelMatrix = Identity(1.0f);

		v3 Translate = Chunk->Translation + CameraOffsetFromHero;
		mat4 TranslationMatrix = Translation(Translate);
		mat4 Matrix = *ViewRotation * TranslationMatrix;
		
		mat4 MVP = *Projection * Matrix * ModelMatrix;
		if(FrustumCulling(MVP, 0.0f, World->ChunkDimInMeters))
		{
			glUniformMatrix4fv(glGetUniformLocation(Shader, "Model"), 1, GL_FALSE, ModelMatrix.Elements);
			glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, Matrix.Elements);
			glBindVertexArray(Chunk->VAO);
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)Chunk->VertexBuffer->size());
		}
	}
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
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ, stack_allocator *WorldAllocator = 0)
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
		
		if(!Chunk->NextChunk && WorldAllocator)
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

	if(Chunk == 0 && WorldAllocator)
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
	block *Blocks = Chunk->ChunkData->Blocks;
	uint32 *Colors = Chunk->ChunkData->Colors;
	occlusion *Occlusions = Chunk->ChunkData->Occlusions;
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

					v3 Color = GetRGBColorFromUInt32(GetVoxel(Colors, XI, YI, ZI));
					A.Color = B.Color = C.Color = D.Color = Color;

					if ((XI == 0) || !GetVoxel(Blocks, XI - 1, YI, ZI).Active)
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X, Y, Z + BlockDimInMeters);
						C.Pos = V3(X, Y + BlockDimInMeters, Z);
						D.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(-1.0f, 0.0f, 0.0f);
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Left;
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
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Right;
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
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Back;
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
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Front;
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
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Bottom;
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
						real32 ThisFaceRayContribution = GetVoxel(Occlusions, XI, YI, ZI).Top;
						ThisFaceRayContribution /= World->RaySample.Top;
						A.Occlusion = B.Occlusion = C.Occlusion = D.Occlusion = ThisFaceRayContribution;
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}
				}
			}
		}
	}
}

#if 0
internal bool32
CanAddTree(world_chunk *Chunk, uint32 X, uint32 Y, uint32 Z)
{
	bool32 Result = true;

	Result = (Y >= 4) && (Y <= 10) && Result;
	if (Result)
	{
		Result = GetVoxel(Chunk->ChunkData->Blocks, X, Y - 1, Z).Active && Result;
		Result = !GetVoxel(Chunk->ChunkData->Blocks, X, Y + 1, Z).Active && Result;
		Result = !GetVoxel(Chunk->ChunkData->Blocks, X, Y + 2, Z).Active && Result;
		Result = !GetVoxel(Chunk->ChunkData->Blocks, X, Y + 3, Z).Active && Result;
		Result = !GetVoxel(Chunk->ChunkData->Blocks, X, Y + 4, Z).Active && Result;
		Result = !GetVoxel(Chunk->ChunkData->Blocks, X, Y + 5, Z).Active && Result;
	}

	return(Result);
}
#endif

inline void
GetChunkData(world *World, world_chunk *Chunk, stack_allocator *WorldAllocator, HANDLE Semaphore)
{
	WaitForSingleObjectEx(Semaphore, INFINITE, false);

	if (World->FirstFreeChunkData)
	{
		Chunk->ChunkData = World->FirstFreeChunkData;
		World->FirstFreeChunkData = World->FirstFreeChunkData->Next;
	}
	else
	{
		Chunk->ChunkData = PushStruct(WorldAllocator, chunk_data);
	}

	ReleaseSemaphore(Semaphore, 1, 0);
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
	// Chunk->FirstBlock.LowEntityCount = 0;
	Chunk->IsNotEmpty = false;
	GetChunkData(World, Chunk, WorldAllocator, WorldAllocatorSemaphore);
	block *Blocks = Chunk->ChunkData->Blocks;
	real32 BlockDimInMeters = World->BlockDimInMeters;
	real32 MaxDistanceInY = World->ChunkDimInMeters * 4; // NOTE(georgy): Max chunks in Y = 4, not counting chunks with Y < 0

	for (uint32 Z = 0; Z < CHUNK_DIM; Z++)
	{
		for (uint32 Y = 0; Y < CHUNK_DIM; Y++)
		{
			for (uint32 X = 0; X < CHUNK_DIM; X++)
			{	
				real32 fY;
				if(Chunk->ChunkY < 0)
				{
					Chunk->IsNotEmpty = true;
					GetVoxel(Blocks, X, Y, Z).Active = true;
					fY = 0.0f;
				}
				else
				{
					fY = (Chunk->ChunkY*World->ChunkDimInMeters + Y*World->BlockDimInMeters) / MaxDistanceInY;
					real32 PlateauFalloff;
					if(fY <= 0.05f)
					{
						PlateauFalloff = 4.0f;
					}
					else if(fY <= 0.4f) 
					{
						PlateauFalloff = 1.0f;
					}
					else if (fY < 0.8f)
					{
						PlateauFalloff = 1.0f - (fY - 0.4f)*2.5f;
					}
					else
					{
						PlateauFalloff = 0.0f;
					}

					real32 Noise = octave_noise_3d(4.0f, 0.5f, 0.007f,
												   (real32)((Chunk->ChunkX * CHUNK_DIM) + X),
												   (real32)((Chunk->ChunkY * CHUNK_DIM) + Y) + 20,
												   (real32)((Chunk->ChunkZ * CHUNK_DIM) + Z));
					Noise = (Noise + 1.0f) * 0.5f;

					Noise = pow(Noise, 2.0f);
					Noise *= PlateauFalloff;

					real32 Caves = raw_noise_3d((real32)((Chunk->ChunkX * CHUNK_DIM) + X) * 0.01f,
												(real32)(((Chunk->ChunkY * CHUNK_DIM) + Y) + 20) * 0.01f,
												(real32)((Chunk->ChunkZ * CHUNK_DIM) + Z) * 0.01f);
					Caves = (Caves + 1.0f) * 0.5f;
					if (Caves < 0.3f)
					{
						Noise = 0.0f;
					}
					if(Noise > 0.35f)
					{
						GetVoxel(Blocks, X, Y, Z).Active = true;
						Chunk->IsNotEmpty = true;
					}
				}

				if(GetVoxel(Blocks, X, Y, Z).Active)
				{
					real32 ColorNoise = octave_noise_3d(4.0f, 0.3f, 0.05f, (real32)((Chunk->ChunkX * CHUNK_DIM) + X), 
																		   (real32)((Chunk->ChunkY * CHUNK_DIM) + Y), 
																		   (real32)((Chunk->ChunkZ * CHUNK_DIM) + Z));
					ColorNoise = (ColorNoise + 1.0f) * 0.5f;

					real32 r, g, b;
					block_type BlockType;
					GetColorAndBlockType(World, fY, ColorNoise, &r, &g, &b, &BlockType);
					GetVoxel(Chunk->ChunkData->BlockTypes, X, Y, Z) = BlockType;
					GetVoxel(Chunk->ChunkData->Colors, X, Y, Z) = GetUInt32Color(r, g, b);

					for (uint32 RayIndex = 0; RayIndex < RAY_COUNT; RayIndex++)
					{
						bool32 Collided = false;
						for (uint32 OffsetIndex = 0; OffsetIndex < POINTS_ON_RAY; OffsetIndex++)
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
							if (GetVoxel(Blocks, XOff, YOff, ZOff).Active)
							{
								Collided = true;
								break;
							}
						}
						if (!Collided)
						{
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Left += World->RaySample.Rays[RayIndex].Left;
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Right += World->RaySample.Rays[RayIndex].Right;
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Top += World->RaySample.Rays[RayIndex].Top;
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Bottom += World->RaySample.Rays[RayIndex].Bottom;
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Front += World->RaySample.Rays[RayIndex].Front;
							GetVoxel(Chunk->ChunkData->Occlusions, X, Y, Z).Back += World->RaySample.Rays[RayIndex].Back;
						}
					}
				}
			}
		}
	}

#if 0
	for (uint32 Z = 0; Z < CHUNK_DIM; Z++)
	{
		for (uint32 Y = 0; Y < CHUNK_DIM; Y++)
		{
			for (uint32 X = 0; X < CHUNK_DIM; X++)
			{
				if(CanAddTree(Chunk, X, Y, Z))
				{
					if((rand() % 200) == 0)
					{
						v3 TreeOffset;
						TreeOffset.x = (real32)X / (real32)CHUNK_DIM * World->ChunkDimInMeters;
						TreeOffset.y = (real32)Y / (real32)CHUNK_DIM * World->ChunkDimInMeters;
						TreeOffset.z = (real32)Z / (real32)CHUNK_DIM * World->ChunkDimInMeters;
						world_position TreePos = {Chunk->ChunkX, Chunk->ChunkY, Chunk->ChunkZ, TreeOffset};
						AddLowEntity(World, WorldAllocator, EntityType_Tree, TreePos);
					}
				}
			}
		}
	}
#endif

	GenerateChunkVertices(World, Chunk, BlockDimInMeters);

	Chunk->IsSetup = true;
	Chunk->IsLoaded = false;
	Chunk->IsModified = false;
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

// TODO(georgy): Should I use job system here? Maybe not because we don't update chunks often
//				 If I want to use job system, I must relocate OpenGL calls from here
internal void
UpdateChunk(world *World, world_chunk *Chunk)
{
	block *Blocks = Chunk->ChunkData->Blocks;
	Chunk->IsNotEmpty = false;
	for (uint32 Z = 0; Z < CHUNK_DIM && !Chunk->IsNotEmpty; Z++)
	{
		for (uint32 Y = 0; Y < CHUNK_DIM && !Chunk->IsNotEmpty; Y++)
		{
			for (uint32 X = 0; X < CHUNK_DIM && !Chunk->IsNotEmpty; X++)
			{
				Chunk->IsNotEmpty = GetVoxel(Blocks, X, Y, Z).Active;
			}
		}
	}

	Chunk->VertexBuffer->resize(0);
	GenerateChunkVertices(World, Chunk, World->BlockDimInMeters);

	glBindVertexArray(Chunk->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_vertex)*Chunk->VertexBuffer->size(), &(*Chunk->VertexBuffer)[0], GL_STATIC_DRAW);
	glBindVertexArray(0);

	Chunk->IsModified = false;
}

internal void
UpdateChunks(world *World)
{
	for (world_chunk *Chunk = World->ChunksUpdateList; Chunk; Chunk = Chunk->NextChunkUpdate)
	{
		UpdateChunk(World, Chunk);
	}
}

internal void
ChangeEntityLocation(world *World, stack_allocator *WorldAllocator, uint32 EntityIndex, low_entity *LowEntity, world_position NewPInit)
{
	world_position *OldP = 0;
	world_position *NewP = 0;

	if(IsValid(LowEntity->P))
	{
		OldP = &LowEntity->P;
	}
	
	if(IsValid(NewPInit))
	{
		NewP = &NewPInit;
	}

	Assert(!OldP || IsValid(*OldP));
	Assert(!NewP || IsValid(*NewP));

	if(OldP && NewP && AreInTheSameChunk(World, OldP, NewP))
	{

	}
	else
	{
		if(OldP)
		{
			world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, WorldAllocator);
			Assert(Chunk);
			world_entity_block *FirstBlock = &Chunk->FirstBlock; 
			bool32 NotFound = true;
			for(world_entity_block *Block = FirstBlock; Block && NotFound; Block = Block->Next)
			{
				for(uint32 Index = 0; Index < Block->LowEntityCount && NotFound; Index++)
				{
					if(Block->LowEntityIndex[Index] == EntityIndex)
					{
						Assert(FirstBlock->LowEntityCount > 0);
						Block->LowEntityIndex[Index] = FirstBlock->LowEntityIndex[--FirstBlock->LowEntityCount];

						if(FirstBlock->LowEntityCount == 0)
						{
							if(FirstBlock->Next)
							{
								world_entity_block *NextBlock = FirstBlock->Next;
								*FirstBlock = *NextBlock;

								NextBlock->Next = World->FirstFreeEntityBlock;
								World->FirstFreeEntityBlock = NextBlock;
							}
						}

						NotFound = false;
					}
				} 
			}
		}

		if(NewP)
		{
			world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, WorldAllocator);
			Assert(Chunk);

			world_entity_block *Block = &Chunk->FirstBlock;
			if(Block->LowEntityCount == ArrayCount(Block->LowEntityIndex))
			{
				world_entity_block *OldBlock = World->FirstFreeEntityBlock;
				if(OldBlock)
				{
					World->FirstFreeEntityBlock = OldBlock->Next;
				}
				else
				{
					OldBlock = PushStruct(WorldAllocator, world_entity_block);
				}

				*OldBlock = *Block;
				Block->Next = OldBlock;
				Block->LowEntityCount = 0;
			}

			Assert(Block->LowEntityCount < ArrayCount(Block->LowEntityIndex));
			Block->LowEntityIndex[Block->LowEntityCount++] = EntityIndex;
		}
	}

	if(NewP)
	{
		LowEntity->P = *NewP;
	}
	else
	{
		LowEntity->P = InvalidPosition();
	}
}
