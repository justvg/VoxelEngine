#pragma once

#define CHUNK_DIM 16
struct world_position
{
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	v3 Offset;
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
	std::vector<block_vertex> *VertexBuffer;

	bool32 IsLoaded;
	bool32 IsModified;

	GLuint VAO, VBO;

	v3 Translation;

	world_chunk *NextChunk;
};

struct world
{
	real32 ChunkDimInMeters;
	real32 BlockDimInMeters;

	real32 MaxNoiseValue;
	real32 MinNoiseValue;
	FastNoise Noise;

	world_chunk *ChunksToRender;

	// NOTE(georgy): Must be a power of two!
	world_chunk *ChunkHash[4096];
};

inline v3
Substract(world *World, world_position *A, world_position *B)
{
	v3 dChunk = { A->ChunkX - B->ChunkX,
				  A->ChunkY - B->ChunkY,
				  A->ChunkZ - B->ChunkZ };

	v3 Result = World->ChunkDimInMeters*dChunk + (A->Offset - B->Offset);

	return(Result);
}

internal void
RenderChunks(world *World, GLuint Shader, mat4 *ViewRotation)
{
	for(world_chunk *Chunk = World->ChunksToRender; Chunk; Chunk = Chunk->NextChunk)
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
GenerateChunkVertices(world_chunk *Chunk, real32 BlockDimInMeters)
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

					if ((XI == 0) || !GetVoxel(Blocks, XI - 1, YI, ZI).Active)
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X, Y, Z + BlockDimInMeters);
						C.Pos = V3(X, Y + BlockDimInMeters, Z);
						D.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(-1.0f, 0.0f, 0.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if ((XI == CHUNK_DIM-1) || !GetVoxel(Blocks, XI + 1, YI, ZI).Active)
					{
						A.Pos = V3(X + BlockDimInMeters, Y, Z);
						B.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						C.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(1.0f, 0.0f, 0.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if ((ZI == 0) || !GetVoxel(Blocks, XI, YI, ZI - 1).Active)
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X, Y + BlockDimInMeters, Z);
						C.Pos = V3(X + BlockDimInMeters, Y, Z);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, 1.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((ZI == CHUNK_DIM-1) || !GetVoxel(Blocks, XI, YI, ZI+1).Active)
					{
						A.Pos = V3(X, Y, Z + BlockDimInMeters);
						B.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						C.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, -1.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((YI == 0) || (!GetVoxel(Blocks, XI, YI-1, ZI).Active))
					{
						A.Pos = V3(X, Y, Z);
						B.Pos = V3(X + BlockDimInMeters, Y, Z);
						C.Pos = V3(X, Y, Z + BlockDimInMeters);
						D.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
						A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, -1.0f, 0.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}

					if((YI == CHUNK_DIM - 1) || (!GetVoxel(Blocks, XI, YI+1, ZI).Active))
					{
						A.Pos = V3(X, Y + BlockDimInMeters, Z);
						B.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
						C.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
						D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
                        A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 1.0f, 0.0f);
						AddQuad(Chunk->VertexBuffer, &A, &B, &C, &D);
					}
				}
			}
		}
	}
}

internal void
LoadChunk(world *World, stack_allocator *WorldAllocator, world_chunk *Chunk)
{
	Chunk->Blocks = PushArray(WorldAllocator, CHUNK_DIM*CHUNK_DIM*CHUNK_DIM, block);
	block *Blocks = Chunk->Blocks;
	real32 BlockDimInMeters = World->BlockDimInMeters;
	for (uint32 Z = 0; Z < CHUNK_DIM; Z++)
	{
		for (uint32 X = 0; X < CHUNK_DIM; X++)
		{	
			real32 NoiseValue = (World->Noise.GetNoise((CHUNK_DIM * Chunk->ChunkX) + X, (CHUNK_DIM * Chunk->ChunkZ) + Z));
			if (NoiseValue > World->MaxNoiseValue) World->MaxNoiseValue = NoiseValue;
			if(NoiseValue < World->MinNoiseValue) World->MinNoiseValue = NoiseValue;
			real32 Height = (NoiseValue + 1.0f) * World->ChunkDimInMeters * 0.5f;
			Assert(Height > 0.0f);
			Assert(Height <= World->ChunkDimInMeters);
			for (uint32 Y = 0; Y < Height; Y++)
			{
				GetVoxel(Blocks, X, Y, Z).Active = true;
			}
		}
	}
	
	GenerateChunkVertices(Chunk, BlockDimInMeters);

	glGenVertexArrays(1, &Chunk->VAO);
	glGenBuffers(1, &Chunk->VBO);
	glBindVertexArray(Chunk->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_vertex)*Chunk->VertexBuffer->size(), &(*Chunk->VertexBuffer)[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*3));
	glBindVertexArray(0);

	Chunk->IsLoaded = true;
}

internal void
UpdateChunk(world *World, world_chunk *Chunk)
{
	Chunk->VertexBuffer->resize(0);
	
	block *Blocks = Chunk->Blocks;
	real32 BlockDimInMeters = World->BlockDimInMeters;

	GenerateChunkVertices(Chunk, BlockDimInMeters);

	glBindVertexArray(Chunk->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Chunk->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_vertex)*Chunk->VertexBuffer->size(), &(*Chunk->VertexBuffer)[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void *)(sizeof(real32)*3));
	glBindVertexArray(0);
}

internal void
FreeChunk(world *World, world_chunk *Chunk)
{

}