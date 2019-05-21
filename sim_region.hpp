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
	sim_region *SimRegion = PushStruct(Allocator, sim_region);
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	SimRegion->Bounds = Bounds;
	
	world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, Bounds.Min);
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, Bounds.Max);
	for (int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ChunkZ++)
	{
		for (int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ChunkY++)
		{
			for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ChunkX++)
			{
				world_chunk *Chunk = GetWorldChunk(World, WorldAllocator, ChunkX, ChunkY, ChunkZ);
				real64 A1 = glfwGetTime();
				if (Chunk)
				{
					if (!Chunk->IsLoaded)
					{
						LoadChunk(World, WorldAllocator, Chunk);
					}
					else if (Chunk->IsModified)
					{
						UpdateChunk(World, Chunk);
					}
					Chunk->NextChunk = World->ChunksToRender;
					World->ChunksToRender = Chunk;
				}
				A1 = glfwGetTime() - A1;
				std::cout << A1 << std::endl;
			}
		}
	}
	

	return(SimRegion);
}

internal void
EndSimulation(world *World)
{
	World->ChunksToRender = 0;
}