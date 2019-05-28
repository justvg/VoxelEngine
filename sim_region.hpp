#pragma once

struct sim_region
{
	world *World;

	world_position Origin;
	rect3 Bounds;
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
	SimRegion->Bounds = Bounds;
	
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
					else if (Chunk->IsSetup && !Chunk->IsLoaded && (LoadChunksPerFrame < MaxLoadChunksPerFrame))
					{
						LoadChunksPerFrame++;
						Chunk->NextChunkLoad = World->ChunksLoadList;
						World->ChunksLoadList = Chunk;
					}
					else if(Chunk->IsSetup && Chunk->IsLoaded && Chunk->IsNotEmpty)
					{
						world_chunk *ChunkToRender = PushStruct(Allocator, world_chunk);
						*ChunkToRender = *Chunk;
						world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ, V3(0.0f, 0.0f, 0.0f)};
						ChunkToRender->Translation = Substract(World, &ChunkPosition, &SimRegion->Origin);
						ChunkToRender->NextChunk = World->ChunksRenderList;
						World->ChunksRenderList = ChunkToRender;
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