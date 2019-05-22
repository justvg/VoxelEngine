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
		for (int32 ChunkY = MinChunkP.ChunkY; ChunkY <= 0; ChunkY++)
		{
			for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ChunkX++)
			{
				world_chunk *Chunk = GetWorldChunk(World, WorldAllocator, ChunkX, ChunkY, ChunkZ);
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
					world_chunk *RenderChunk = PushStruct(Allocator, world_chunk);
					*RenderChunk = *Chunk;
					world_position ChunkPosition = {ChunkX, ChunkY, ChunkZ, V3(0.0f, 0.0f, 0.0f)};
					RenderChunk->Translation = Substract(World, &ChunkPosition, &SimRegion->Origin);
					RenderChunk->NextChunk = World->ChunksToRender;
					World->ChunksToRender = RenderChunk;
				}
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