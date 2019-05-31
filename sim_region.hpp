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
				world_chunk *Chunk = GetWorldChunk(World, WorldAllocator, ChunkX, ChunkY, ChunkZ);
				if (Chunk)
				{
					if (!Chunk->IsSetup && (SetupChunksPerFrame < MaxSetupChunksPerFrame))
					{
						SetupChunksPerFrame++;
						Chunk->NextChunkSetup = World->ChunksSetupList;
						World->ChunksSetupList = Chunk;
					}
					else if (Chunk->IsSetup && Chunk->IsNotEmpty && !Chunk->IsLoaded && (LoadChunksPerFrame < MaxLoadChunksPerFrame))
					{
						LoadChunksPerFrame++;
						Chunk->NextChunkLoad = World->ChunksLoadList;
						World->ChunksLoadList = Chunk;
					}
					else if(Chunk->IsSetup && Chunk->IsLoaded)
					{
						if(Chunk->IsNotEmpty)
						{
							world_chunk *ChunkToRender = PushStruct(Allocator, world_chunk);
							*ChunkToRender = *Chunk;
							world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ, V3(0.0f, 0.0f, 0.0f)};
							ChunkToRender->Translation = Substract(World, &ChunkPosition, &SimRegion->Origin);
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
								// TODO(georgy): Change this test to testing entity volume and sim bounds, not a point and sim bounds!
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

struct mesh
{
	GLuint VAO, VBO;
	uint32 VerticesCount;
};

struct mesh_load_info
{
	char *Filename;
};

struct graphics_assets
{
	stack_allocator AssetsAllocator;

	mesh *EntityModels[EntityType_Count];
	mesh_load_info Infos[EntityType_Count];
};

internal graphics_assets *
InitializeGameAssets(stack_allocator *Allocator, memory_size Size)
{
	graphics_assets *Assets = PushStruct(Allocator, graphics_assets);
	SubMemory(&Assets->AssetsAllocator, Allocator, Size);

	Assets->Infos[EntityType_Hero].Filename = "";
	Assets->Infos[EntityType_Tree].Filename = "";

	return(Assets);
}

internal void
LoadAsset(graphics_assets *Assets, entity_type Type)
{
	Assets->EntityModels[Type] = PushStruct(&Assets->AssetsAllocator, mesh);

// TODO(georgy): Implement asset loader!
#if 0
	LoadMesh(Assets, Type, Assets->Infos[Type].Filename);
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
MoveEntity(sim_entity *Entity, real32 DeltaTime, v3 ddP, real32 Speed)
{
	ddP *= Speed;
	ddP += -1.5f*Entity->dP;
	Entity->P = (0.5f*ddP*DeltaTime*DeltaTime) + (Entity->dP*DeltaTime) + Entity->P;
	Entity->dP = ddP*DeltaTime + Entity->dP;
}

internal void
UpdateAndRenderEntities(world *World, sim_region *SimRegion, graphics_assets *Assets, hero_control *Hero, 
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
					v3 ddP = Hero->ddP;
					MoveEntity(Entity, DeltaTime, ddP, 10.0f);
				};
			}

			if(Assets->EntityModels[Entity->Type])
			{
				v3 Translate = Entity->P + CameraOffsetFromHero;
				mat4 TranslationMatrix = Translation(Translate);
				mat4 Matrix = *ViewRotation * TranslationMatrix;
			
				glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, Matrix.Elements);
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