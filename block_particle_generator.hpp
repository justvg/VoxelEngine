#pragma once

struct block_particle
{
	v3 P, dP;
	v3 Color;
	real32 LifeTime;
};

struct block_particle_generator
{
	uint32 LastUsedParticle;
	block_particle Particles[512];

	shader Shader;
	GLuint VAO, VBO;
};

inline void
InitializeBlockParticleGenerator(block_particle_generator *Generator, shader Shader)
{
	uint32 LastUsedParticle = 0;
	for (uint32 I = 0; I < ArrayCount(Generator->Particles); I++)
	{
		Generator->Particles[I].LifeTime = -1.0f;
	}

	Generator->Shader = Shader;

	real32 CubeVertices[] = {
		// Positions          // normals
		0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
		-0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
		0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 
		-0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
		0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
		-0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f, 

		-0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
		0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
		0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
		0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
		-0.1f,  0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  
		-0.1f, -0.1f,  0.1f,  0.0f,  0.0f, 1.0f,  

		-0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 
		-0.1f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
		-0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
		-0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f, 
		-0.1f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 
		-0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f, 

		0.1f,  0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
		0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f,
		0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f, 
		0.1f, -0.1f,  0.1f,  1.0f,  0.0f,  0.0f,
		0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
		0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f, 

		-0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 
		0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 
		0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
		0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
		-0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f, 
		-0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f, 

		0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
		-0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
		0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f, 
		-0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
		0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
		-0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f
	};

	glGenVertexArrays(1, &Generator->VAO);
	glGenBuffers(1, &Generator->VBO);
	glBindVertexArray(Generator->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Generator->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(real32), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(real32), (void *)(3*sizeof(real32)));
	glBindVertexArray(0);
}

internal uint32
UnusedParticleIndex(block_particle_generator *Generator)
{
	for (uint32 I = Generator->LastUsedParticle + 1; I < ArrayCount(Generator->Particles); I++)
	{
		if (Generator->Particles[I].LifeTime <= 0.0f)
		{
			Generator->LastUsedParticle = I;
			return(I);
		}
	}

	for (uint32 I = 0; I < (Generator->LastUsedParticle + 1); I++)
	{
		if (Generator->Particles[I].LifeTime <= 0.0f)
		{
			Generator->LastUsedParticle = I;
			return(I);
		}
	}

	std::cout << "Too many block particles!" << std::endl;

	Generator->LastUsedParticle = 0;
	return(0);
}

inline void
AddParticle(block_particle_generator *Generator, v3 P, v3 Color)
{
	uint32 Index = UnusedParticleIndex(Generator);
	block_particle *Particle = Generator->Particles + Index;

	Particle->P = P;
	v3 MainDir = V3(0.0f, 1.0f, 0.0f);
	v3 RandomDir = V3(((rand() % 200) - 100.0f) / 100.0f, ((rand() % 200) - 100.0f) / 100.0f, ((rand() % 200) - 100.0f) / 100.0f);
	Particle->dP = 2.5f*(MainDir + RandomDir);
	Particle->Color = Color;
	Particle->LifeTime = 1.0f;
}

internal void
AddParticles(block_particle_generator *Generator, v3 P, v3 Color, uint32 Count)
{
	for (uint32 I = 0; I < Count; I++)
	{
		AddParticle(Generator, P, Color);
	}
}

internal void
UpdateParticleGenerator(block_particle_generator *Generator, real32 DeltaTime)
{
	for (uint32 I = 0; I < ArrayCount(Generator->Particles); I++)
	{
		block_particle *Particle = Generator->Particles + I;
		Particle->LifeTime -= DeltaTime;
		if (Particle->LifeTime > 0.0f)
		{
			Particle->dP += V3(0.0f, -9.8f, 0.0f) * DeltaTime;
			Particle->P += Particle->dP * DeltaTime;
		}
		else
		{
			Particle->LifeTime = -1.0f;
		}
	}
}

internal void
DrawParticles(block_particle_generator *Generator, mat4 *ViewMatrix)
{
	Generator->Shader.Enable();
	Generator->Shader.SetMat4("View", *ViewMatrix);

	glBindVertexArray(Generator->VAO);
	for (uint32 I = 0; I < ArrayCount(Generator->Particles); I++)
	{
		block_particle *Particle = Generator->Particles + I;
		if (Particle->LifeTime > 0.0f)
		{
			mat4 ModelMatrix = Translation(Particle->P);
			Generator->Shader.SetMat4("Model", ModelMatrix);
			Generator->Shader.SetVec3("inColor", Particle->Color);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
	glBindVertexArray(0);
}