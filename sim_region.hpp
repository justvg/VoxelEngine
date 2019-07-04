#pragma 

struct sim_region
{
	world *World;

	world_position Origin;
	rect3 UpdatableBounds;
	rect3 Bounds;

	uint32 MaxEntityCount;
	uint32 EntityCount;
	sim_entity *Entities;

	// NOTE(georgy): Must be power of two!
	entity_reference Hash[4096];
};

internal entity_reference *
GetHashFromStorageIndex(sim_region *SimRegion, uint32 StorageIndex)
{
	Assert(StorageIndex);

	entity_reference *Result = 0;

	uint32 HashValue = StorageIndex;
	for(uint32 Offset = 0; Offset < ArrayCount(SimRegion->Hash); Offset++)
	{
		uint32 HashMask = ArrayCount(SimRegion->Hash) - 1;
		uint32 HashIndex = (HashValue + Offset) & HashMask;
		entity_reference *Entry = SimRegion->Hash + HashIndex;
		if((Entry->LowIndex == 0) || (Entry->LowIndex == StorageIndex))
		{
			Result = Entry;
			break;
		}
	}

	return(Result);
}

inline v3
GetSimSpaceP(sim_region *SimRegion, low_entity *LowEntity)
{
	// TODO(georgy): Formulate this!
	v3 Result = V3((real32)TILE_CHUNK_SAFE_MARGIN, (real32)TILE_CHUNK_SAFE_MARGIN, (real32)TILE_CHUNK_SAFE_MARGIN);
	if(!LowEntity->Sim.NonSpatial)
	{
		Result = Substract(SimRegion->World, &LowEntity->P, &SimRegion->Origin);
	}

	return(Result);
}

internal sim_entity *
AddEntity(sim_region *SimRegion, low_entity *LowEntity, v3 SimSpaceP);
inline void
LoadEntityReference(sim_region *SimRegion, entity_reference *Ref)
{
	if(Ref->LowIndex && !Ref->SimPtr)
	{
		low_entity *LowEntity = SimRegion->World->LowEntities + Ref->LowIndex;
		v3 SimSpaceP = GetSimSpaceP(SimRegion, LowEntity);
		Ref->SimPtr = AddEntity(SimRegion, LowEntity, SimSpaceP);
	}
}

internal sim_entity *
AddEntity(sim_region *SimRegion, low_entity *LowEntity, v3 SimSpaceP)
{
	sim_entity *Entity = 0;

	entity_reference *Entry = GetHashFromStorageIndex(SimRegion, LowEntity->Sim.StorageIndex);
	if(Entry->SimPtr == 0)
	{
		if (SimRegion->EntityCount < SimRegion->MaxEntityCount)
		{
			Entity = SimRegion->Entities + SimRegion->EntityCount++;
			LowEntity->Sim.P = SimSpaceP;
			LowEntity->Sim.Updatable = IsInRect(SimRegion->UpdatableBounds, SimSpaceP);

			*Entity = LowEntity->Sim;
			
			Entry->LowIndex = Entity->StorageIndex;
			Entry->SimPtr = Entity;

			LoadEntityReference(SimRegion, &Entity->Fireball);
		}
	}

	return(Entity);
}

internal sim_region *
BeginSimulation(stack_allocator *Allocator, stack_allocator *WorldAllocator, world *World, world_position Origin, rect3 Bounds)
{
	local_persist uint32 TESTframenumber = 0;

	uint32 MaxSetupChunksPerFrame = 4;
	uint32 SetupChunksPerFrame = 0;

	uint32 MaxLoadChunksPerFrame = 4;
	uint32 LoadChunksPerFrame = 0;

	uint32 MaxUpdateChunksPerFrame = 4;
	uint32 UpdateChunksPerFrame = 0;

	sim_region *SimRegion = PushStruct(Allocator, sim_region);
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	SimRegion->UpdatableBounds = Bounds;
	ZeroMemory(SimRegion->Hash, ArrayCount(SimRegion->Hash)*sizeof(entity_reference));
	// TODO(georgy): Need more accurate values here!
	real32 MaxEntityRadius = 5.0f;
	real32 MaxEntityVelocity = 5.0f;
	real32 UpdateSafetyRadiusXZ = MaxEntityRadius + MaxEntityVelocity + 1.0f;
	real32 UpdateSafetyRadiusY = 1.0f;
	SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, V3(UpdateSafetyRadiusXZ, UpdateSafetyRadiusY, UpdateSafetyRadiusXZ));

	SimRegion->MaxEntityCount = 4096;
	SimRegion->EntityCount = 0;
	SimRegion->Entities = PushArray(Allocator, SimRegion->MaxEntityCount, sim_entity);
	
	// TODO(georgy): Add 1 to all chunks to include offset
	world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, Bounds.Min);
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, Bounds.Max);
	for (int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ChunkZ++)
	{
		for (int32 ChunkY = -1; ChunkY <= 3; ChunkY++)
		{
			for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ChunkX++)
			{
				world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ, WorldAllocator);
				if (Chunk)
				{
					if(Chunk->IsSetup && !IsRecentlyUsed(World->RecentlyUsedChunksList, Chunk))
					{
						Chunk->NextChunkUsed = World->RecentlyUsedChunksList;
						World->RecentlyUsedChunksList = Chunk;
					}

					if (!Chunk->IsSetup && (SetupChunksPerFrame < MaxSetupChunksPerFrame))
					{
						SetupChunksPerFrame++;
						Chunk->NextChunkSetup = World->ChunksSetupList;
						World->ChunksSetupList = Chunk;
					}
					else if(Chunk->IsSetup && Chunk->IsModified && (UpdateChunksPerFrame < MaxUpdateChunksPerFrame))
					{
						UpdateChunksPerFrame++;
						Chunk->NextChunkUpdate = World->ChunksUpdateList;
						World->ChunksUpdateList = Chunk;
					}
					else if (Chunk->IsSetup && Chunk->IsNotEmpty && !Chunk->IsLoaded && (LoadChunksPerFrame < MaxLoadChunksPerFrame))
					{
						LoadChunksPerFrame++;
						Chunk->NextChunkLoad = World->ChunksLoadList;
						World->ChunksLoadList = Chunk;
					}
					
					if(Chunk->IsSetup)
					{
						if (Chunk->IsNotEmpty)
						{
							world_chunk *ChunkToRender = PushStruct(Allocator, world_chunk);
							*ChunkToRender = *Chunk;
							world_position ChunkPosition = { ChunkX, ChunkY, ChunkZ, V3(0.0f, 0.0f, 0.0f) };
							ChunkToRender->Translation = Substract(World, &ChunkPosition, &SimRegion->Origin);
							Chunk->Translation = ChunkToRender->Translation;
							ChunkToRender->LengthSquaredToOrigin = LengthSq(ChunkToRender->Translation);
							ChunkToRender->NextChunk = World->ChunksRenderList;
							World->ChunksRenderList = ChunkToRender;
						}

						for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
						{
							for(uint32 EntityIndexInBlock = 0; EntityIndexInBlock < Block->LowEntityCount; EntityIndexInBlock++)
							{
								uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndexInBlock];
								low_entity *LowEntity = World->LowEntities + LowEntityIndex;
								v3 SimSpaceP = Substract(World, &LowEntity->P, &SimRegion->Origin);
								// TODO(georgy): Change this test to testing entity volume and sim bound, not a point and sim bound!
								if(!LowEntity->Sim.NonSpatial)
								{
									if(IsInRect(SimRegion->Bounds, SimSpaceP))
									{
										AddEntity(SimRegion, LowEntity, SimSpaceP);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	TESTframenumber++;
	if(TESTframenumber == 600)
	{
		TESTframenumber = 0;
		UnloadChunks(World, MinChunkP, MaxChunkP);
	}


	return(SimRegion);
}

internal void
EndSimulation(sim_region *SimRegion, world *World, stack_allocator *WorldAllocator)
{
	World->ChunksSetupList = 0;
	World->ChunksLoadList = 0;
	World->ChunksUpdateList = 0;
	World->ChunksRenderList = 0;

	for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;
		Entity->Fireball.SimPtr = 0;
		low_entity *LowEntity = World->LowEntities + Entity->StorageIndex;
		LowEntity->Sim = *Entity;

		world_position NewP = MapIntoChunkSpace(World, SimRegion->Origin, Entity->P);
		ChangeEntityLocation(World, WorldAllocator, Entity->StorageIndex, LowEntity, NewP);
	}
}

internal void
LoadAsset(graphics_assets *Assets, asset_type Type)
{
	Assets->EntityModels[Type] = PushStruct(&Assets->AssetsAllocator, mesh);

#if 1
	LoadQubicleBinary(Assets->EntityModels[Type], Assets->Infos[Type].Filename);
#else
	switch(Type)
	{
		case EntityType_Hero:
		{
			real32 TestCubeVertices[] = {
				// Positions          
				0.5f, -0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  
				0.5f,  0.5f, -0.5f,  
				-0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f, -0.5f,  
				-0.5f, -0.5f, -0.5f,  

				-0.5f, -0.5f,  0.5f,
				0.5f, -0.5f,  0.5f,
				0.5f,  0.5f,  0.5f,
				0.5f,  0.5f,  0.5f,
				-0.5f,  0.5f,  0.5f,
				-0.5f, -0.5f,  0.5f,

				-0.5f,  0.5f,  0.5f,
				-0.5f,  0.5f, -0.5f,
				-0.5f, -0.5f, -0.5f,
				-0.5f, -0.5f, -0.5f,
				-0.5f, -0.5f,  0.5f,
				-0.5f,  0.5f,  0.5f, 

				0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f,  0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  

				-0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f, -0.5f,  
				0.5f, -0.5f,  0.5f,  
				0.5f, -0.5f,  0.5f,  
				-0.5f, -0.5f,  0.5f,  
				-0.5f, -0.5f, -0.5f,  

				0.5f,  0.5f, -0.5f,  
				-0.5f,  0.5f, -0.5f,  
				0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f,  0.5f,  
				0.5f,  0.5f,  0.5f,  
				-0.5f,  0.5f, -0.5f
			};
			glGenVertexArrays(1, &Assets->EntityModels[Type]->VAO);
			glGenBuffers(1,&Assets->EntityModels[Type]->VBO);
			glBindVertexArray(Assets->EntityModels[Type]->VAO);
			glBindBuffer(GL_ARRAY_BUFFER, Assets->EntityModels[Type]->VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(TestCubeVertices), TestCubeVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
			glBindVertexArray(0);

			Assets->EntityModels[Type]->VerticesCount = sizeof(TestCubeVertices);
		} break;
	}
#endif
}

internal void
GetBlockIndicesAndPos(world *World, world_chunk *Chunk, world_position WorldP, uint32 *X, uint32 *Y, uint32 *Z, v3 *BlockPos)
{
	*X = (uint32)((WorldP.Offset.x / World->ChunkDimInMeters) * CHUNK_DIM);
	*Y = (uint32)((WorldP.Offset.y / World->ChunkDimInMeters) * CHUNK_DIM);
	*Z = (uint32)((WorldP.Offset.z / World->ChunkDimInMeters) * CHUNK_DIM);

	BlockPos->x = Chunk->Translation.x + (*X*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);
	BlockPos->y = Chunk->Translation.y + (*Y*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);
	BlockPos->z = Chunk->Translation.z + (*Z*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);
}

internal bool32
IsBlockActive(world *World, world_chunk *Chunk, world_position WorldP, v3 *BlockPos)
{
	uint32 X, Y, Z;
	GetBlockIndicesAndPos(World, Chunk, WorldP, &X, &Y, &Z, BlockPos);
	
	bool32 Result = GetVoxel(Chunk->ChunkData->Blocks, X, Y, Z).Active;
	return(Result);
}

internal void
SetBlockActive(block_particle_generator *BlockParticleGenerator, world *World, world_chunk *Chunk, world_position WorldP, bool32 Active)
{
	uint32 X, Y, Z;
	v3 BlockPos;
	GetBlockIndicesAndPos(World, Chunk, WorldP, &X, &Y, &Z, &BlockPos);

	if(GetVoxel(Chunk->ChunkData->Blocks, X, Y, Z).Active != Active)
	{
		GetVoxel(Chunk->ChunkData->Blocks, X, Y, Z).Active = Active;\
		Chunk->IsModified = true;

		if (!Active)
		{
			v3 Color = GetRGBColorFromUInt32(GetVoxel(Chunk->ChunkData->Colors, X, Y, Z));
			AddParticles(BlockParticleGenerator, BlockPos, Color, 4);
		}
	}
}

inline void
MakeEntityNonSpatial(sim_entity *Entity)
{
	Entity->NonSpatial = true;
	// TODO(georgy): Formulate this!
	Entity->P = V3((real32)TILE_CHUNK_SAFE_MARGIN, (real32)TILE_CHUNK_SAFE_MARGIN, (real32)TILE_CHUNK_SAFE_MARGIN);
}

internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelZ, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaZ, real32 PlayerDeltaY,
         real32 *tMin, real32 MinZ, real32 MaxZ, real32 MinY, real32 MaxY)
{
    bool32 Result = false;
    real32 tEpsilon = 0.05f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
			real32 Z = RelZ + tResult*PlayerDeltaZ;
			if((Z >= MinZ) && (Z <= MaxZ))
            {
				real32 Y = RelY + tResult*PlayerDeltaY;
				if((Y >= MinY) && (Y <= MaxY))
				{
					*tMin = Maximum(0.0f, tResult - tEpsilon);
					Result = true;
				}
            }
        }
    }
    return(Result);
}

internal bool32
CanCollide(sim_entity *A, sim_entity *B)
{
	bool32 Result = false;

	if (A != B)
	{
		if (A->Collides && B->Collides)
		{
			Result = true;
		}

		if (A->Type > B->Type)
		{
			sim_entity *Temp = A;
			A = B;
			B = Temp;
		}

		if(A->Type == EntityType_Hero && B->Type == EntityType_Fireball)
		{
			Result = false;
		}
	}

	return(Result);
}

internal bool32 
HandleCollision(sound_system *SoundSystem, sim_region *SimRegion, v3 DistancePassed, world_position *OldWorldP, block_particle_generator *BlockParticleGenerator,
				sim_entity *EntityA, sim_entity *EntityB, entity_type A, entity_type B)
{
	bool32 Result = false;

	if (A > B)
	{
		entity_type Temp = A;
		A = B;
		B = Temp;
	}

	if (A == EntityType_Chunk && B == EntityType_Fireball)
	{
		Assert(EntityA);
		Assert(EntityA->Type == EntityType_Fireball);
		MakeEntityNonSpatial(EntityA);
		SoundSystem->StopSoundAndDeleteFromHashTable(EntityA->StorageIndex);
		for (int32 Z = -2; Z <= 2; Z++)
		{
			for (int32 Y = -2; Y <= 2; Y++)
			{
				for (int32 X = -2; X <= 2; X++)
				{
					v3 Offset = { DistancePassed.x + X*SimRegion->World->BlockDimInMeters, DistancePassed.y + Y*SimRegion->World->BlockDimInMeters, DistancePassed.z + Z*SimRegion->World->BlockDimInMeters };
					world_position WorldP = MapIntoChunkSpace(SimRegion->World, *OldWorldP, Offset);
					world_chunk *Chunk = GetWorldChunk(SimRegion->World, WorldP.ChunkX, WorldP.ChunkY, WorldP.ChunkZ);
					Assert(Chunk);
					SetBlockActive(BlockParticleGenerator, SimRegion->World, Chunk, WorldP, false);
				}
			}
		}
	}
	else if (A == EntityType_Tree && B == EntityType_Fireball)
	{
		Assert(EntityA);
		Assert(EntityA->Type == EntityType_Fireball);
		MakeEntityNonSpatial(EntityA);
	}
	else if (A == EntityType_Chunk && B == EntityType_Hero)
	{
		Result = true;
	}
	else if (A == EntityType_Hero && B == EntityType_Tree)
	{
		Result = true;
	}

	return(Result);
}

struct move_spec
{
	v3 ddP;
	real32 Speed;
	real32 Drag;
	bool32 GravityAffected;
};
internal void
MoveEntity(sim_region *SimRegion, sound_system *SoundSystem, block_particle_generator *BlockParticleGenerator, 
		   sim_entity *Entity, real32 DeltaTime, move_spec *MoveSpec)
{
	real32 ddPLength = LengthSq(MoveSpec->ddP);
	if(ddPLength > 1.0f)
	{
		MoveSpec->ddP *= 1.0f / sqrtf(ddPLength);
	}
	MoveSpec->ddP *= MoveSpec->Speed;
	v3 DragV = -MoveSpec->Drag*Entity->dP;
	DragV.y = 0.0f;
	MoveSpec->ddP += DragV;

	world_position OldWorldP = SimRegion->World->LowEntities[Entity->StorageIndex].P;
#if 1
	v3 PointsToCheck[4];
	PointsToCheck[0] = V3(-0.5f*Entity->Collision->TotalVolume.Dim.x, -0.51f*Entity->Collision->TotalVolume.Dim.y, -0.5f*Entity->Collision->TotalVolume.Dim.z) +
								Entity->Collision->TotalVolume.OffsetP;
	PointsToCheck[1] = V3(-0.5f*Entity->Collision->TotalVolume.Dim.x, -0.51f*Entity->Collision->TotalVolume.Dim.y, 0.5f*Entity->Collision->TotalVolume.Dim.z) +
								Entity->Collision->TotalVolume.OffsetP;;
	PointsToCheck[2] = V3(0.5f*Entity->Collision->TotalVolume.Dim.x, -0.51f*Entity->Collision->TotalVolume.Dim.y, -0.5f*Entity->Collision->TotalVolume.Dim.z) +
								Entity->Collision->TotalVolume.OffsetP;
	PointsToCheck[3] = V3(0.5f*Entity->Collision->TotalVolume.Dim.x, -0.51f*Entity->Collision->TotalVolume.Dim.y, 0.5f*Entity->Collision->TotalVolume.Dim.z) +
								Entity->Collision->TotalVolume.OffsetP;
	bool32 Active = false;
	for (uint32 I = 0; !Active && (I < 4); I++)
	{
		world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, PointsToCheck[I]);
		world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
		Assert(Chunk);
		v3 BlockP;
		Active = IsBlockActive(SimRegion->World, Chunk, NewWorldP, &BlockP);
	}
	if (!Active && MoveSpec->GravityAffected)
	{
		MoveSpec->ddP += V3(0.0f, -9.8f, 0.0f);
		Entity->OnGround = false;
	}
	else
	{
		Entity->OnGround = true;
	}
#endif
	
	v3 PreviousPos = Entity->P;
	v3 CheckPos = (0.5f*MoveSpec->ddP*DeltaTime*DeltaTime) + (Entity->dP*DeltaTime) + PreviousPos;
	v3 EntityDelta = CheckPos - PreviousPos;

	Entity->dP += MoveSpec->ddP*DeltaTime;

	real32 DistanceRemaining = Entity->DistanceLimit;
	if (DistanceRemaining == 0.0f)
	{
		DistanceRemaining = 10000.0f;
	}

	for(uint32 Iteration = 0; Iteration < 4; Iteration++)
	{
		real32 EntityDeltaLength = Length(EntityDelta);
		if (EntityDeltaLength > 0.0f)
		{
			real32 tMin = 1.0f;
			if (EntityDeltaLength > DistanceRemaining)
			{
				tMin = DistanceRemaining / EntityDeltaLength;
			}
			v3 DesiredPos = Entity->P + EntityDelta;
			v3 Normal = {};

			sim_entity *HitEntity = 0;
			entity_type HitEntityType = EntityType_Null;
			if(!Entity->NonSpatial)
			{ 
				// NOTE(georgy): Chunk collision test. Should/Can I make chunk an entity? 
				for (int32 Z = -2; Z <= 2; Z++)
				{
					for (int32 Y = -2; Y <= 2; Y++)
					{
						for (int32 X = -2; X <= 2; X++)
						{
							v3 Offset = { EntityDelta.x + X*SimRegion->World->BlockDimInMeters, 
										  EntityDelta.y + Y*SimRegion->World->BlockDimInMeters, 
										  EntityDelta.z + Z*SimRegion->World->BlockDimInMeters };
							world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, Offset);
							world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
							Assert(Chunk);
							v3 BlockP;
							bool32 Active = IsBlockActive(SimRegion->World, Chunk, NewWorldP, &BlockP);
							if(Active)
							{
								v3 MinkowskiDiameter = {SimRegion->World->BlockDimInMeters + Entity->Collision->TotalVolume.Dim.x,
														SimRegion->World->BlockDimInMeters + Entity->Collision->TotalVolume.Dim.y,
														SimRegion->World->BlockDimInMeters + Entity->Collision->TotalVolume.Dim.z};

								v3 MinCorner = -0.5f*MinkowskiDiameter;
								v3 MaxCorner = 0.5f*MinkowskiDiameter;

								v3 RelPlayerP = (Entity->P + Entity->Collision->TotalVolume.OffsetP) - BlockP;

								if(TestWall(MinCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y, 
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(-1.0f, 0.0f, 0.0f);
									HitEntityType = EntityType_Chunk;
								}
								if(TestWall(MaxCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y, 
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(1.0f, 0.0f, 0.0f);
									HitEntityType = EntityType_Chunk;
								}
								if(TestWall(MinCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(0.0f, 0.0f, -1.0f);
									HitEntityType = EntityType_Chunk;
								}
								if(TestWall(MaxCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(0.0f, 0.0f, 1.0f);
									HitEntityType = EntityType_Chunk;
								}
								if (TestWall(MinCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
								{
									Normal = V3(0.0f, -1.0f, 0.0f);
									HitEntityType = EntityType_Chunk;
								}
								if (TestWall(MaxCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
								{
									Normal = V3(0.0f, 1.0f, 0.0f);
									HitEntityType = EntityType_Chunk;
								}
							}
						}
					}
				}

				// NOTE(georgy): Entities collision
				for (uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; TestEntityIndex++)
				{
					sim_entity *TestEntity = SimRegion->Entities + TestEntityIndex;

					if (CanCollide(Entity, TestEntity))
					{
						for (uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++)
						{
							sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
							for (uint32 TestVolumeIndex = 0; TestVolumeIndex < TestEntity->Collision->VolumeCount; TestVolumeIndex++)
							{
								sim_entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
						
								v3 MinkowskiDiameter = { TestVolume->Dim.x + Volume->Dim.x,
														 TestVolume->Dim.y + Volume->Dim.y,
														 TestVolume->Dim.z + Volume->Dim.z };

								v3 MinCorner = -0.5f*MinkowskiDiameter;
								v3 MaxCorner = 0.5f*MinkowskiDiameter;

								v3 RelPlayerP = (Entity->P + Volume->OffsetP) - (TestEntity->P + TestVolume->OffsetP);

								if (TestWall(MinCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(-1.0f, 0.0f, 0.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
								if (TestWall(MaxCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(1.0f, 0.0f, 0.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
								if (TestWall(MinCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(0.0f, 0.0f, -1.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
								if (TestWall(MaxCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
								{
									Normal = V3(0.0f, 0.0f, 1.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
								if (TestWall(MinCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
								{
									Normal = V3(0.0f, -1.0f, 0.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
								if (TestWall(MaxCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
									&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
								{
									Normal = V3(0.0f, 1.0f, 0.0f);
									HitEntity = TestEntity;
									HitEntityType = TestEntity->Type;
								}
							}
						}
					}
				}

				v3 DistancePassed = tMin*EntityDelta;
				Entity->P += DistancePassed;
				DistanceRemaining -= tMin*EntityDeltaLength;
				EntityDelta = DesiredPos - Entity->P;
				if(HitEntityType)
				{
					bool32 StopOnCollision = HandleCollision(SoundSystem, SimRegion, DistancePassed, &OldWorldP, BlockParticleGenerator,
															 Entity, HitEntity, Entity->Type, HitEntityType);
					if (StopOnCollision)
					{
						Entity->dP = Entity->dP - Dot(Entity->dP, Normal)*Normal;
						EntityDelta = EntityDelta - Dot(EntityDelta, Normal)*Normal;
					}
				}

				OldWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, DistancePassed);
			}
		}
	}

	if (Entity->DistanceLimit != 0.0f)
	{
		Entity->DistanceLimit = DistanceRemaining;
	}
}

internal void
DrawModel(graphics_assets *Assets, GLuint Shader, asset_type AssetType,
		  real32 ScaleValue, real32 Rotation, v3 Translate)
{
	if(Assets->EntityModels[AssetType])
	{
		mat4 ModelMatrix = Scale(ScaleValue);
		ModelMatrix = Rotate(Rotation, V3(0.0f, 1.0f, 0.0f)) * ModelMatrix;
		ModelMatrix = Translation(Translate) * ModelMatrix;
		glUniformMatrix4fv(glGetUniformLocation(Shader, "Model"), 1, GL_FALSE, ModelMatrix.Elements);
		glBindVertexArray(Assets->EntityModels[AssetType]->VAO);
		glDrawArrays(GL_TRIANGLES, 0, Assets->EntityModels[AssetType]->VerticesCount);
	}
	else
	{
		LoadAsset(Assets, AssetType);
	}
}

internal void
UpdateAndRenderEntities(sim_region *SimRegion, game *Game,
						real32 DeltaTime, GLuint Shader, mat4 *ViewRotation, v3 CameraOffsetFromHero)
{
	graphics_assets *Assets = Game->Assets;
	hero_control *Hero = &Game->HeroControl;
	block_particle_generator *BlockParticleGenerator = &Game->BlockParticleGenerator;
	sound_system *SoundSystem = &Game->Sound;
	for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;	

		if(Entity->Updatable)
		{
			move_spec MoveSpec = {};
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					v3 ActionDir = Normalize(V3(CameraOffsetFromHero.x, 0, CameraOffsetFromHero.z));
					MoveSpec.ddP = Hero->ddP;
					MoveSpec.Speed = 10.0f;
					MoveSpec.Drag = 1.5f;
					MoveSpec.GravityAffected = true;
					if (Hero->dY && Entity->OnGround)
					{
						Entity->dP.y = Hero->dY;
						SoundSystem->PlaySound2D(Sound_Jump);
					}
					if (Hero->Attack)
					{
						world_position OldWorldP = SimRegion->World->LowEntities[Entity->StorageIndex].P;
						world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, ActionDir);
						world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
						Assert(Chunk);
						SetBlockActive(BlockParticleGenerator, SimRegion->World, Chunk, NewWorldP, false);
						SoundSystem->PlayRandomSound2D(Sound_Hit1, Sound_Hit3);
					}
					if (Hero->Fireball)
					{
						sim_entity *Fireball = Entity->Fireball.SimPtr;
						if (Fireball && Fireball->NonSpatial)
						{
							Fireball->DistanceLimit = 15.0f;
							Fireball->NonSpatial = false;
							Fireball->P = Entity->P;
							Fireball->dP = V3(Entity->dP.x, 0.0f, Entity->dP.z) + 10.0f*ActionDir;
							SoundSystem->PlaySound2DAndAddToHashTable(Fireball->StorageIndex, Sound_Spell);
						}
					}
				} break;
				case EntityType_Fireball:
				{
					if (Entity->DistanceLimit == 0.0f)
					{
						MakeEntityNonSpatial(Entity);
						SoundSystem->StopSoundAndDeleteFromHashTable(Entity->StorageIndex);
					}
				} break;
			}

			if(!Entity->NonSpatial && Entity->Moveable)
			{
				MoveEntity(SimRegion, SoundSystem, BlockParticleGenerator, Entity, DeltaTime, &MoveSpec);
			}

			mat4 ModelMatrix = Identity(1.0f);
			v3 Translate = Entity->P + CameraOffsetFromHero;
			mat4 TranslationMatrix = Translation(Translate);
			mat4 ViewMatrix = *ViewRotation * TranslationMatrix;
			glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, ViewMatrix.Elements);

			switch (Entity->Type)
			{
				case EntityType_Hero:
				{
					real32 HeroRotRadians = Hero->Rot / 180.0f * M_PI;
					Entity->FacingDir.x = sinf(HeroRotRadians + M_PI);
					Entity->FacingDir.z = cosf(HeroRotRadians + M_PI);
					v3 Right = Normalize(Cross(Entity->FacingDir, V3(0.0f, 1.0f, 0.0f)));

					v3 TranslateLeftFoot = Assets->Infos[Asset_HeroLeftFoot].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroLeftFoot].Offset.y, 0.0f);
					v3 TranslateRightFoot = Assets->Infos[Asset_HeroRightFoot].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroRightFoot].Offset.y, 0.0f);
					v3 TranslateLeftHand = Assets->Infos[Asset_HeroLeftHand].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroLeftHand].Offset.y, 0.0f);
					v3 TranslateRightHand = Assets->Infos[Asset_HeroRightHand].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroRightHand].Offset.y, 0.0f);
					v3 TranslateLeftShoulder = Assets->Infos[Asset_HeroLeftShoulder].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroLeftShoulder].Offset.y, 0.0f);
					v3 TranslateRightShoulder = Assets->Infos[Asset_HeroRightShoulder].Offset.x * Right + V3(0.0f, Assets->Infos[Asset_HeroRightShoulder].Offset.y, 0.0f);

					real32 EntitydPLength = Length(Entity->dP);
					v3 AnimOffsetSin = EntitydPLength / 6 * 0.06f*sinf(10.0f*glfwGetTime()) * Entity->FacingDir;
					v3 AnimOffsetCos = EntitydPLength / 6 * 0.06f*cosf(10.0f*glfwGetTime()) * Entity->FacingDir;

					DrawModel(Assets, Shader, Asset_HeroHead, Assets->Infos[Asset_HeroHead].Scale,
							  Hero->Rot + 180.0f, Assets->Infos[Asset_HeroHead].Offset);
					DrawModel(Assets, Shader, Asset_HeroBody, Assets->Infos[Asset_HeroBody].Scale,
							  Hero->Rot + 180.0f, Assets->Infos[Asset_HeroBody].Offset);
					DrawModel(Assets, Shader, Asset_HeroLegs, Assets->Infos[Asset_HeroLegs].Scale,
							  Hero->Rot + 180.0f, Assets->Infos[Asset_HeroLegs].Offset);
					DrawModel(Assets, Shader, Asset_HeroLeftFoot, Assets->Infos[Asset_HeroLeftFoot].Scale,
							  Hero->Rot + 180.0f, TranslateLeftFoot + AnimOffsetSin);
					DrawModel(Assets, Shader, Asset_HeroRightFoot, Assets->Infos[Asset_HeroRightFoot].Scale,
							  Hero->Rot + 180.0f, TranslateRightFoot + AnimOffsetCos);
					DrawModel(Assets, Shader, Asset_HeroLeftHand, Assets->Infos[Asset_HeroLeftHand].Scale,
							  Hero->Rot + 180.0f, TranslateLeftHand + AnimOffsetCos);
					DrawModel(Assets, Shader, Asset_HeroRightHand, Assets->Infos[Asset_HeroRightHand].Scale,
							  Hero->Rot + 180.0f, TranslateRightHand + AnimOffsetSin);
					DrawModel(Assets, Shader, Asset_HeroLeftShoulder, Assets->Infos[Asset_HeroLeftShoulder].Scale,
							  Hero->Rot + 180.0f, TranslateLeftShoulder + AnimOffsetCos);
					DrawModel(Assets, Shader, Asset_HeroRightShoulder, Assets->Infos[Asset_HeroRightShoulder].Scale,
							  Hero->Rot + 180.0f, TranslateRightShoulder + AnimOffsetSin);

					if (!Game->HeroCollision)
					{
						Game->HeroCollision = AllocateCollision(&Game->WorldAllocator, 9);

						Game->HeroCollision->Volumes[Asset_HeroHead - Asset_HeroHead].Dim = 
							Assets->Infos[Asset_HeroHead].Scale*GetRectDim(Assets->EntityModels[Asset_HeroHead]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroHead - Asset_HeroHead].OffsetP = Assets->Infos[Asset_HeroHead].Offset;

						Game->HeroCollision->Volumes[Asset_HeroBody - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroBody].Scale*GetRectDim(Assets->EntityModels[Asset_HeroBody]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroBody - Asset_HeroHead].OffsetP = Assets->Infos[Asset_HeroBody].Offset;

						Game->HeroCollision->Volumes[Asset_HeroLegs - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroLegs].Scale*GetRectDim(Assets->EntityModels[Asset_HeroLegs]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroLegs - Asset_HeroHead].OffsetP = Assets->Infos[Asset_HeroLegs].Offset;

						Game->HeroCollision->Volumes[Asset_HeroLeftFoot - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroLeftFoot].Scale*GetRectDim(Assets->EntityModels[Asset_HeroLeftFoot]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroLeftFoot - Asset_HeroHead].OffsetP = TranslateLeftFoot;

						Game->HeroCollision->Volumes[Asset_HeroRightFoot - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroRightFoot].Scale*GetRectDim(Assets->EntityModels[Asset_HeroRightFoot]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroRightFoot - Asset_HeroHead].OffsetP = TranslateRightFoot;

						Game->HeroCollision->Volumes[Asset_HeroLeftHand - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroLeftHand].Scale*GetRectDim(Assets->EntityModels[Asset_HeroLeftHand]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroLeftHand - Asset_HeroHead].OffsetP = TranslateLeftHand;

						Game->HeroCollision->Volumes[Asset_HeroRightHand - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroRightHand].Scale*GetRectDim(Assets->EntityModels[Asset_HeroRightFoot]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroRightHand - Asset_HeroHead].OffsetP = TranslateRightHand;

						Game->HeroCollision->Volumes[Asset_HeroLeftShoulder - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroLeftShoulder].Scale*GetRectDim(Assets->EntityModels[Asset_HeroLeftShoulder]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroLeftShoulder - Asset_HeroHead].OffsetP = TranslateLeftShoulder;

						Game->HeroCollision->Volumes[Asset_HeroRightShoulder - Asset_HeroHead].Dim =
							Assets->Infos[Asset_HeroRightShoulder].Scale*GetRectDim(Assets->EntityModels[Asset_HeroRightShoulder]->AABB);
						Game->HeroCollision->Volumes[Asset_HeroLeftShoulder - Asset_HeroHead].OffsetP = TranslateRightShoulder;

						ComputeTotalVolume(Game->HeroCollision, Entity->P);
						if (Game->HeroCollision->TotalVolume.Dim.z > Game->HeroCollision->TotalVolume.Dim.x)
						{
							Game->HeroCollision->TotalVolume.Dim.x = Game->HeroCollision->TotalVolume.Dim.z;
						}
						else
						{
							Game->HeroCollision->TotalVolume.Dim.z = Game->HeroCollision->TotalVolume.Dim.x;
						}
					}
					Entity->Collision = Game->HeroCollision;
				} break;
				case EntityType_Fireball:
				{
					DrawModel(Assets, Shader, Asset_Fireball, Assets->Infos[Asset_Fireball].Scale,
							  0.0f, Assets->Infos[Asset_Fireball].Offset);
					if (!Game->FireballCollision)
					{
						v3 Dim = Assets->Infos[Asset_Fireball].Scale*(Assets->EntityModels[Asset_Fireball]->AABB.Max - Assets->EntityModels[Asset_Fireball]->AABB.Min);
						v3 Offset = Assets->Infos[Asset_Fireball].Offset;
						Game->FireballCollision = MakeSimpleCollision(&Game->WorldAllocator, Dim, Offset);
					}
					Entity->Collision = Game->FireballCollision;
				} break;
				case EntityType_Tree:
				{
					DrawModel(Assets, Shader, Asset_Tree, Assets->Infos[Asset_Tree].Scale,
							  0.0f, Assets->Infos[Asset_Tree].Offset);
					if (!Game->TreeCollision)
					{
						v3 Dim = Assets->Infos[Asset_Tree].Scale*(Assets->EntityModels[Asset_Tree]->AABB.Max - Assets->EntityModels[Asset_Tree]->AABB.Min);
						v3 Offset = Assets->Infos[Asset_Tree].Offset;
						Game->TreeCollision = MakeSimpleCollision(&Game->WorldAllocator, Dim, Offset);
					}
					Entity->Collision = Game->TreeCollision;
				} break;
			}
		}
	}
} 