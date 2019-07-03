#pragma once

enum asset_type
{
	Asset_None,

	Asset_HeroHead,
	Asset_HeroBody,
	Asset_HeroLegs,
	Asset_HeroLeftFoot,
	Asset_HeroRightFoot,
	Asset_HeroLeftHand,
	Asset_HeroRightHand,
	Asset_HeroLeftShoulder,
	Asset_HeroRightShoulder,

	Asset_Tree,

	Asset_Fireball,

	Asset_Count
};

struct mesh
{
	GLuint VAO, VBO;
	uint32 VerticesCount;
	rect3 AABB;
};

struct mesh_load_info
{
	char *Filename;
	real32 Scale;
	v3 Offset;
};

struct graphics_assets
{
	stack_allocator AssetsAllocator;

	mesh *EntityModels[Asset_Count];
	mesh_load_info Infos[Asset_Count];
};

internal graphics_assets *
InitializeGameAssets(stack_allocator *Allocator, memory_size Size)
{
	graphics_assets *Assets = PushStruct(Allocator, graphics_assets);
	SubMemory(&Assets->AssetsAllocator, Allocator, Size);

	Assets->Infos[Asset_HeroHead].Filename = "data/models/MaleHead01.qb";
	Assets->Infos[Asset_HeroHead].Scale = 0.04f;
	Assets->Infos[Asset_HeroHead].Offset = V3(0.0f, 0.27f, 0.0f);

	Assets->Infos[Asset_HeroBody].Filename = "data/models/Body01.qb";
	Assets->Infos[Asset_HeroBody].Scale = 0.09f;
	Assets->Infos[Asset_HeroBody].Offset = V3(0.0f, 0.02f, 0.0f);

	Assets->Infos[Asset_HeroLegs].Filename = "data/models/Legs01.qb";
	Assets->Infos[Asset_HeroLegs].Scale = 0.09f;
	Assets->Infos[Asset_HeroLegs].Offset = V3(0.0f, -0.29f, 0.0f);

	Assets->Infos[Asset_HeroLeftFoot].Filename = "data/models/LeftFoot01.qb";
	Assets->Infos[Asset_HeroLeftFoot].Scale = 0.09f;
	Assets->Infos[Asset_HeroLeftFoot].Offset = V3(-0.13f, -0.44f, 0.0f);

	Assets->Infos[Asset_HeroRightFoot].Filename = "data/models/RightFoot01.qb";
	Assets->Infos[Asset_HeroRightFoot].Scale = 0.09f;
	Assets->Infos[Asset_HeroRightFoot].Offset = V3(0.13f, -0.44f, 0.0f);

	Assets->Infos[Asset_HeroLeftHand].Filename = "data/models/LeftHand01.qb";
	Assets->Infos[Asset_HeroLeftHand].Scale = 0.09f;
	Assets->Infos[Asset_HeroLeftHand].Offset = V3(-0.265f, -0.175f, 0.0f);

	Assets->Infos[Asset_HeroRightHand].Filename = "data/models/RightHand01.qb";
	Assets->Infos[Asset_HeroRightHand].Scale = 0.09f;
	Assets->Infos[Asset_HeroRightHand].Offset = V3(0.265f, -0.175f, 0.0f);

	Assets->Infos[Asset_HeroLeftShoulder].Filename = "data/models/LeftShoulders01.qb";
	Assets->Infos[Asset_HeroLeftShoulder].Scale = 0.09f;
	Assets->Infos[Asset_HeroLeftShoulder].Offset = V3(-0.25f, 0.02f, 0.0f);

	Assets->Infos[Asset_HeroRightShoulder].Filename = "data/models/RightShoulders01.qb";
	Assets->Infos[Asset_HeroRightShoulder].Scale = 0.09f;
	Assets->Infos[Asset_HeroRightShoulder].Offset = V3(0.25f, 0.02f, 0.0f);

	Assets->Infos[Asset_Tree].Filename = "data/models/tree1.qb";
	Assets->Infos[Asset_Tree].Scale = 0.35f;
	Assets->Infos[Asset_Tree].Offset = V3(0.0f, 0.0f, 0.0f);

	Assets->Infos[Asset_Fireball].Filename = "data/models/Knight.qb";
	Assets->Infos[Asset_Fireball].Scale = 0.05f;
	Assets->Infos[Asset_Fireball].Offset = V3(0.0f, 0.0f, 0.0f);

	return(Assets);
}

struct qb_header
{
	uint32 Version;
	uint32 ColorFormat;
	uint32 Handedness;
	uint32 Compression;
	uint32 VisibilityMask;
	uint32 MatrixCount;
};

struct qb_matrix
{
	uint8 NameLength;
	char *Name;
	uint32 SizeX, SizeY, SizeZ;
	int32 PosX, PosY, PosZ;
};

struct qb_vertex
{
	v3 Pos;
	v3 Normal;
	v3 Color;
};

internal void
AddQuad(std::vector<qb_vertex> &VertexBuffer, qb_vertex *A, qb_vertex *B, qb_vertex *C, qb_vertex *D)
{
	VertexBuffer.push_back(*A);
	VertexBuffer.push_back(*B);
	VertexBuffer.push_back(*C);
	VertexBuffer.push_back(*C);
	VertexBuffer.push_back(*B);
	VertexBuffer.push_back(*D);
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

inline uint32
ReadColorQB(uint32 ColorQB, uint32 ColorFormat)
{
	uint32 Result;
	if(ColorFormat == 0)
	{
		Result = ColorQB;
	}
	else
	{
		Result = ((ColorQB & 0x00FF0000) >> 16) |
				 ((ColorQB & 0x0000FF00)) |
				 ((ColorQB & 0x000000FF) << 16);
	}

	return(Result);
}

// TODO(georgy): Use visibility mask to merge some faces
internal void
SetVoxel(std::vector<qb_vertex> &VertexBuffer, uint32 XI, uint32 YI, uint32 ZI, real32 BlockDimInMeters, uint32 ColorUInt32, v3 ModelDimensions)
{
	real32 X = BlockDimInMeters*((real32)XI) - 0.5f*ModelDimensions.x*BlockDimInMeters;
	real32 Y = BlockDimInMeters*((real32)YI) - 0.5f*ModelDimensions.y*BlockDimInMeters;
	real32 Z = BlockDimInMeters*((real32)ZI) - 0.5f*ModelDimensions.z*BlockDimInMeters;
	qb_vertex A, B, C, D;

	v3 Color = GetRGBColorFromUInt32(ColorUInt32);
	A.Color = B.Color = C.Color = D.Color = Color;

	{
		A.Pos = V3(X, Y, Z);
		B.Pos = V3(X, Y, Z + BlockDimInMeters);
		C.Pos = V3(X, Y + BlockDimInMeters, Z);
		D.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(-1.0f, 0.0f, 0.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}

	{
		A.Pos = V3(X + BlockDimInMeters, Y, Z);
		B.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
		C.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
		D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(1.0f, 0.0f, 0.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}

	{
		A.Pos = V3(X, Y, Z);
		B.Pos = V3(X, Y + BlockDimInMeters, Z);
		C.Pos = V3(X + BlockDimInMeters, Y, Z);
		D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, -1.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}

	{
		A.Pos = V3(X, Y, Z + BlockDimInMeters);
		B.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
		C.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
		D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 0.0f, 1.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}

	{
		A.Pos = V3(X, Y, Z);
		B.Pos = V3(X + BlockDimInMeters, Y, Z);
		C.Pos = V3(X, Y, Z + BlockDimInMeters);
		D.Pos = V3(X + BlockDimInMeters, Y, Z + BlockDimInMeters);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, -1.0f, 0.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}

	{
		A.Pos = V3(X, Y + BlockDimInMeters, Z);
		B.Pos = V3(X, Y + BlockDimInMeters, Z + BlockDimInMeters);
		C.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z);
		D.Pos = V3(X + BlockDimInMeters, Y + BlockDimInMeters, Z + BlockDimInMeters);
		A.Normal = B.Normal = C.Normal = D.Normal = V3(0.0f, 1.0f, 0.0f);
		AddQuad(VertexBuffer, &A, &B, &C, &D);
	}
}

#define CODEFLAG 2
#define NEXTSLICEFLAG 6

internal void
LoadQubicleBinary(mesh *Mesh, char *Filename)
{
	std::vector<qb_vertex> VertexBuffer;

	uint8 *DataPointer = (uint8 *)ReadEntireFile(Filename);
	qb_header *Header = (qb_header *)DataPointer;
	DataPointer += sizeof(qb_header);
	for(uint32 I = 0; I < Header->MatrixCount; I++)
	{
		qb_matrix MatrixData;
		MatrixData.NameLength = *DataPointer;
		DataPointer++;
		MatrixData.Name = (char *)DataPointer;
		DataPointer += MatrixData.NameLength;
		MatrixData.SizeX = *(uint32 *)DataPointer;
		DataPointer += sizeof(uint32);
		MatrixData.SizeY = *(uint32 *)DataPointer;
		DataPointer += sizeof(uint32);
		MatrixData.SizeZ = *(uint32 *)DataPointer;
		DataPointer += sizeof(uint32);
		MatrixData.PosX = *(int32 *)DataPointer;
		DataPointer += sizeof(int32);
		MatrixData.PosY = *(int32 *)DataPointer;
		DataPointer += sizeof(int32);
		MatrixData.PosZ = *(int32 *)DataPointer;
		DataPointer += sizeof(int32);

		uint32 Data;
		v3 ModelDimensions = V3((real32)MatrixData.SizeX, (real32)MatrixData.SizeY, (real32)MatrixData.SizeZ);
		if(Header->Compression == 0)
		{
			for(uint32 Z = 0; Z < MatrixData.SizeZ; Z++)
			{
				for(uint32 Y = 0; Y < MatrixData.SizeY; Y++)
				{
					for(uint32 X = 0; X < MatrixData.SizeX; X++)
					{
						Data = *(uint32 *)DataPointer;	
						DataPointer += sizeof(uint32);

						if(Data & 0xFF000000)
						{
							uint32 Color = ReadColorQB(Data, Header->ColorFormat);
							SetVoxel(VertexBuffer, X, Y, Z, 0.5f, Color, ModelDimensions);
						}
					}	
				}
			}
		}
		else
		{
			for(uint32 Z = 0; Z < MatrixData.SizeZ; Z++)
			{
				uint32 Index = 0;
				while(true)
				{
					Data = *(uint32 *)DataPointer;	
					DataPointer += sizeof(uint32);

					if(Data == NEXTSLICEFLAG)
					{
						break;
					}
					else if(Data == CODEFLAG)
					{
						uint32 Count = *(uint32 *)DataPointer;	
						DataPointer += sizeof(uint32);
						Data = *(uint32 *)DataPointer;	
						DataPointer += sizeof(uint32);

						for(uint32 J = 0; J < Count; J++)
						{
							uint32 X = Index % MatrixData.SizeX;
							uint32 Y = Index / MatrixData.SizeX;
							Index++;
							
							if(Data & 0xFF000000)
							{
								uint32 Color = ReadColorQB(Data, Header->ColorFormat);
								SetVoxel(VertexBuffer, X, Y, Z, 0.5f, Color, ModelDimensions);
							}
						}
					}
					else
					{
						uint32 X = Index % MatrixData.SizeX;
						uint32 Y = Index / MatrixData.SizeX;
						Index++;

						if(Data & 0xFF000000)
						{
							uint32 Color = ReadColorQB(Data, Header->ColorFormat);
							SetVoxel(VertexBuffer, X, Y, Z, 0.5f, Color, ModelDimensions);
						}
					}
				}
			}
		}
	}

	Mesh->AABB.Min = V3(FLT_MAX, FLT_MAX, FLT_MAX);
	Mesh->AABB.Max = V3(FLT_MIN, FLT_MIN, FLT_MIN);
	for (uint32 I = 0; I < VertexBuffer.size(); I++)
	{
		if (VertexBuffer[I].Pos.x > Mesh->AABB.Max.x) Mesh->AABB.Max.x = VertexBuffer[I].Pos.x;
		if (VertexBuffer[I].Pos.y > Mesh->AABB.Max.y) Mesh->AABB.Max.y = VertexBuffer[I].Pos.y;
		if (VertexBuffer[I].Pos.z > Mesh->AABB.Max.z) Mesh->AABB.Max.z = VertexBuffer[I].Pos.z;
		if (VertexBuffer[I].Pos.x < Mesh->AABB.Min.x) Mesh->AABB.Min.x = VertexBuffer[I].Pos.x;
		if (VertexBuffer[I].Pos.y < Mesh->AABB.Min.y) Mesh->AABB.Min.y = VertexBuffer[I].Pos.y;
		if (VertexBuffer[I].Pos.z < Mesh->AABB.Min.z) Mesh->AABB.Min.z = VertexBuffer[I].Pos.z;
	}

	Mesh->VerticesCount = (uint32)VertexBuffer.size();
	glGenVertexArrays(1, &Mesh->VAO);
	glGenBuffers(1, &Mesh->VBO);
	glBindVertexArray(Mesh->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(qb_vertex)*VertexBuffer.size(), &VertexBuffer[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(qb_vertex), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(qb_vertex), (void *)(sizeof(real32)*3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(qb_vertex), (void *)(sizeof(real32)*6));
	glBindVertexArray(0);
}