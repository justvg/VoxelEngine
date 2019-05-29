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
EndSimulation(world *World)
{
	World->ChunksSetupList = 0;
	World->ChunksLoadList = 0;
	World->ChunksRenderList = 0;
}

struct test_model
{
	GLuint VAO, VBO;
	real32 VertexBuffer[36];
};
internal void
RenderEntities(world *World, sim_region *SimRegion, GLuint Shader, mat4 *ViewRotation, test_model *TestModel)
{
	for(uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; EntityIndex++)
	{
		sim_entity *Entity = SimRegion->Entities + EntityIndex;	

		mat4 TranslationMatrix = Translation(Entity->P);
		mat4 Matrix = *ViewRotation * TranslationMatrix;
			
		glUniformMatrix4fv(glGetUniformLocation(Shader, "View"), 1, GL_FALSE, Matrix.Elements);
		glBindVertexArray(TestModel->VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
} 