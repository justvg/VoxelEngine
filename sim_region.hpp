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
};

internal sim_region *
BeginSimulation(stack_allocator *Allocator, stack_allocator *WorldAllocator, world *World, world_position Origin, rect3 Bounds)
{
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
					else if (Chunk->IsSetup && Chunk->IsNotEmpty && !Chunk->IsModified && !Chunk->IsLoaded && (LoadChunksPerFrame < MaxLoadChunksPerFrame))
					{
						LoadChunksPerFrame++;
						Chunk->NextChunkLoad = World->ChunksLoadList;
						World->ChunksLoadList = Chunk;
					}
					
					if(Chunk->IsSetup)
					{
						if(Chunk->IsNotEmpty)
						{
							world_chunk *ChunkToRender = PushStruct(Allocator, world_chunk);
							*ChunkToRender = *Chunk;
							world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ, V3(0.0f, 0.0f, 0.0f)};
							ChunkToRender->Translation = Substract(World, &ChunkPosition, &SimRegion->Origin);
							Chunk->Translation = ChunkToRender->Translation;
							ChunkToRender->LengthSquaredToOrigin = LengthSq(ChunkToRender->Translation);
							ChunkToRender->NextChunk = World->ChunksRenderList;
							World->ChunksRenderList = ChunkToRender;
						}
					}

					if(Chunk->IsSetup)
					{
						for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
						{
							for(uint32 EntityIndexInBlock = 0; EntityIndexInBlock < Block->LowEntityCount; EntityIndexInBlock++)
							{
								uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndexInBlock];
								low_entity *LowEntity = World->LowEntities + LowEntityIndex;
								v3 SimSpaceP = Substract(World, &LowEntity->P, &SimRegion->Origin);
								// TODO(georgy): Change this test to testing entity volume and sim bound, not a point and sim bound!
								if(IsInRect(SimRegion->Bounds, SimSpaceP))
								{
									if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
									{
										sim_entity *Entity = SimRegion->Entities + SimRegion->EntityCount++;
										LowEntity->Sim.P = SimSpaceP;
										if(IsInRect(SimRegion->UpdatableBounds, SimSpaceP))
										{
											LowEntity->Sim.Updatable = true;
										}

										*Entity = LowEntity->Sim;
									}
								}
							}
						}
					}
				}
			}
		}
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
		low_entity *LowEntity = World->LowEntities + Entity->StorageIndex;

		LowEntity->Sim = *Entity;

		world_position NewP = MapIntoChunkSpace(World, SimRegion->Origin, Entity->P);
		ChangeEntityLocation(World, WorldAllocator, Entity->StorageIndex, LowEntity, NewP);
	}
}

internal void
LoadAsset(graphics_assets *Assets, entity_type Type)
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

internal bool32
IsBlockActive(world *World, world_chunk *Chunk, world_position WorldP, v3 *BlockPos)
{
	uint32 X = (uint32)((WorldP.Offset.x / World->ChunkDimInMeters) * CHUNK_DIM);
	uint32 Y = (uint32)((WorldP.Offset.y / World->ChunkDimInMeters) * CHUNK_DIM);
	uint32 Z = (uint32)((WorldP.Offset.z / World->ChunkDimInMeters) * CHUNK_DIM);
	bool32 Result = GetVoxel(Chunk->Blocks, X, Y, Z).Active;

	BlockPos->x = Chunk->Translation.x + (X*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);
	BlockPos->y = Chunk->Translation.y + (Y*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);
	BlockPos->z = Chunk->Translation.z + (Z*World->BlockDimInMeters + 0.5f*World->BlockDimInMeters);

	return(Result);
}

internal void
SetBlockActive(world *World, world_chunk *Chunk, world_position WorldP, bool32 Active)
{
	uint32 X = (uint32)((WorldP.Offset.x / World->ChunkDimInMeters) * CHUNK_DIM);
	uint32 Y = (uint32)((WorldP.Offset.y / World->ChunkDimInMeters) * CHUNK_DIM);
	uint32 Z = (uint32)((WorldP.Offset.z / World->ChunkDimInMeters) * CHUNK_DIM);

	if(GetVoxel(Chunk->Blocks, X, Y, Z).Active != Active)
	{
		GetVoxel(Chunk->Blocks, X, Y, Z).Active = Active;
		Chunk->IsModified = true;
	}
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

internal void
MoveEntity(sim_region *SimRegion, sim_entity *Entity, real32 DeltaTime, v3 ddP, real32 Speed)
{
	real32 ddPLength = LengthSq(ddP);
	if(ddPLength > 1.0f)
	{
		ddP *= 1.0f / sqrtf(ddPLength); 
	}
	ddP *= Speed;
	v3 Drag = -1.5f*Entity->dP;
	Drag.y = 0.0f;
	ddP += Drag;

	world_position OldWorldP = SimRegion->World->LowEntities[Entity->StorageIndex].P;
#if 1
	v3 PointsToCheck[4];
	PointsToCheck[0] = V3(-0.5f*Entity->Dim.x, -0.51f*Entity->Dim.y, -0.5f*Entity->Dim.z);
	PointsToCheck[1] = V3(-0.5f*Entity->Dim.x, -0.51f*Entity->Dim.y, 0.5f*Entity->Dim.z);
	PointsToCheck[2] = V3(0.5f*Entity->Dim.x, -0.51f*Entity->Dim.y, -0.5f*Entity->Dim.z);
	PointsToCheck[3] = V3(0.5f*Entity->Dim.x, -0.51f*Entity->Dim.y, 0.5f*Entity->Dim.z);
	bool32 Active = false;
	for (uint32 I = 0; !Active && (I < 4); I++)
	{
		world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, PointsToCheck[I]);
		world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
		Assert(Chunk);
		v3 BlockP;
		Active = IsBlockActive(SimRegion->World, Chunk, NewWorldP, &BlockP);
	}
	if (!Active)
	{
		ddP += V3(0.0f, -9.8f, 0.0f);
		Entity->OnGround = false;
	}
	else
	{
		Entity->OnGround = true;
	}
#endif
	
	v3 PreviousPos = Entity->P;
	v3 CheckPos = (0.5f*ddP*DeltaTime*DeltaTime) + (Entity->dP*DeltaTime) + PreviousPos;
	v3 EntityDelta = CheckPos - PreviousPos;

	Entity->dP += ddP*DeltaTime;

	for(uint32 Iteration = 0; Iteration < 4; Iteration++)
	{
		real32 EntityDeltaLength = Length(EntityDelta);
		if (EntityDeltaLength > 0.0f)
		{
			v3 DesiredPos = Entity->P + EntityDelta;
			v3 Normal = {};
			real32 tMin = 1.0f;
			for (int32 Z = -2; Z <= 2; Z++)
			{
				for (int32 Y = -2; Y <= 2; Y++)
				{
					for (int32 X = -2; X <= 2; X++)
					{
						v3 Offset = { EntityDelta.x + X*SimRegion->World->BlockDimInMeters, EntityDelta.y + Y*SimRegion->World->BlockDimInMeters, EntityDelta.z + Z*SimRegion->World->BlockDimInMeters };
						world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, Offset);
						world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
						Assert(Chunk);
						v3 BlockP;
						bool32 Active = IsBlockActive(SimRegion->World, Chunk, NewWorldP, &BlockP);
						if(Active)
						{
							v3 MinkowskiDiameter = {SimRegion->World->BlockDimInMeters + Entity->Dim.x,
													SimRegion->World->BlockDimInMeters + Entity->Dim.y,
													SimRegion->World->BlockDimInMeters + Entity->Dim.z};

							v3 MinCorner = -0.5f*MinkowskiDiameter;
							v3 MaxCorner = 0.5f*MinkowskiDiameter;

							v3 RelPlayerP = Entity->P - BlockP;

							if(TestWall(MinCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y, 
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
							{
								Normal = V3(-1.0f, 0.0f, 0.0f);
							}
							if(TestWall(MaxCorner.x, RelPlayerP.x, RelPlayerP.z, RelPlayerP.y, EntityDelta.x, EntityDelta.z, EntityDelta.y, 
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
							{
								Normal = V3(1.0f, 0.0f, 0.0f);
							}
							if(TestWall(MinCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
							{
								Normal = V3(0.0f, 0.0f, -1.0f);
							}
							if(TestWall(MaxCorner.z, RelPlayerP.z, RelPlayerP.x, RelPlayerP.y, EntityDelta.z, EntityDelta.x, EntityDelta.y,
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.y, MaxCorner.y))
							{
								Normal = V3(0.0f, 0.0f, 1.0f);
							}
							if (TestWall(MinCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
							{
								Normal = V3(0.0f, -1.0f, 0.0f);
							}
							if (TestWall(MaxCorner.y, RelPlayerP.y, RelPlayerP.x, RelPlayerP.z, EntityDelta.y, EntityDelta.x, EntityDelta.z,
								&tMin, MinCorner.x, MaxCorner.x, MinCorner.z, MaxCorner.z))
							{
								Normal = V3(0.0f, 1.0f, 0.0f);
							}
						}
					}
				}
			}

			Entity->P += tMin*EntityDelta;
			EntityDelta = DesiredPos - Entity->P;
			if (Normal.x != 0 || Normal.z != 0 || Normal.y != 0)
			{
				Entity->dP = Entity->dP - Dot(Entity->dP, Normal)*Normal;
				EntityDelta = EntityDelta - Dot(EntityDelta, Normal)*Normal;
			}

			OldWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, tMin*EntityDelta);
		}
	}
}

internal void
UpdateAndRenderEntities(sim_region *SimRegion, graphics_assets *Assets, hero_control *Hero, 
						real32 DeltaTime, GLuint Shader, mat4 *ViewRotation, v3 CameraOffsetFromHero)
{
	for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;	

		if(Entity->Updatable)
		{
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					Entity->FacingDir = Normalize(V3(CameraOffsetFromHero.x, 0, CameraOffsetFromHero.z));
					v3 ddP = Hero->ddP;
					if (Hero->dY && Entity->OnGround)
					{
						Entity->dP.y = Hero->dY;
					}
					if (Hero->Attack)
					{
						world_position OldWorldP = SimRegion->World->LowEntities[Entity->StorageIndex].P;
						world_position NewWorldP = MapIntoChunkSpace(SimRegion->World, OldWorldP, Entity->FacingDir);
						world_chunk *Chunk = GetWorldChunk(SimRegion->World, NewWorldP.ChunkX, NewWorldP.ChunkY, NewWorldP.ChunkZ);
						Assert(Chunk);
						v3 BlockP;
						SetBlockActive(SimRegion->World, Chunk, NewWorldP, false);
					}
					MoveEntity(SimRegion, Entity, DeltaTime, ddP, 10.0f);
				} break;
			}

			if(Assets->EntityModels[Entity->Type])
			{
				mat4 ModelMatrix = Identity(1.0f);
				switch(Entity->Type)
				{
					case EntityType_Hero:
					{
						ModelMatrix = Scale(0.11f);
						ModelMatrix = ModelMatrix * Rotate(Hero->Rot + 180.0f, V3(0.0f, 1.0f, 0.0f));
					} break;
					case EntityType_Tree:
					{
						ModelMatrix = Translation(V3(0.0f, 1.3f, 0.0f));
						ModelMatrix = ModelMatrix * Scale(0.35f);
					} break;
				}
				
				v3 Translate = Entity->P + CameraOffsetFromHero;
				mat4 TranslationMatrix = Translation(Translate);
				mat4 ViewMatrix = *ViewRotation * TranslationMatrix;
				glUniformMatrix4fv(glGetUniformLocation(Shader, "Model"), 1, GL_FALSE, ModelMatrix.Elements);
				glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, ViewMatrix.Elements);
				glBindVertexArray(Assets->EntityModels[Entity->Type]->VAO);
				glDrawArrays(GL_TRIANGLES, 0, Assets->EntityModels[Entity->Type]->VerticesCount);
			}
			else
			{
				LoadAsset(Assets, Entity->Type);
			}
		}
	}
} 