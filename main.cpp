#include "engine.h"

internal void
GLFWKeyCallback(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods)
{
	game_input *Input = (game_input *)glfwGetWindowUserPointer(Window);

	if (Key == GLFW_KEY_W)
	{
		Input->MoveForward = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_S)
	{
		Input->MoveBack = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_A)
	{
		Input->MoveLeft = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_D)
	{
		Input->MoveRight = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_SPACE)
	{
		Input->MoveUp = (Action == GLFW_PRESS);
	}
}

internal void
GLFWMouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	game_input *Input = (game_input *)glfwGetWindowUserPointer(Window);

	if (Button == GLFW_MOUSE_BUTTON_1)
	{
		Input->MouseLeft = (Action != GLFW_RELEASE);
	}
	if (Button == GLFW_MOUSE_BUTTON_2)
	{
		Input->MouseRight = (Action == GLFW_PRESS);
	}
}

internal void
GLFWCursorPositionCallback(GLFWwindow* Window, double XPos, double YPos)
{
	game_input *Input = (game_input *)glfwGetWindowUserPointer(Window);

	Input->MouseX = (int32)XPos;
	Input->MouseY = (int32)YPos;
}

internal void
ProcessInput(game_input *Input, camera *Camera, hero_control *Hero, real32 DeltaTime)
{
	int32 X, Y;
	X = Input->MouseX;
	Y = Input->MouseY;
	static int32 LastMouseX = 0;
	static int32 LastMouseY = 0;
	static bool32 FirstMouse = true;
	if (FirstMouse)
	{
		LastMouseX = X;
		LastMouseY = Y;
		FirstMouse = false;
	}

	real32 XOffset = (real32)((X - LastMouseX)*Camera->RotSensetivity);
	real32 YOffset = (real32)((LastMouseY - Y)*Camera->RotSensetivity);

	LastMouseX = X;
	LastMouseY = Y;

	Camera->Pitch += YOffset;
	Camera->Head += XOffset;

	Camera->Pitch = Camera->Pitch > 89.0f ? 89.0f : Camera->Pitch;
	Camera->Pitch = Camera->Pitch < -89.0f ? -89.0f : Camera->Pitch;

	v3 HeroMovementDirection = Normalize(V3(-Camera->OffsetFromHero.x, 0, -Camera->OffsetFromHero.z));
	real32 Theta = atan2f(-HeroMovementDirection.z, HeroMovementDirection.x) * 180.0f / M_PI;

	v3 CameraUp = {0.0f, 1.0f, 0.0f};
	Hero->ddP = {};
	Hero->dY = 0.0f;
	if(Input->MoveForward)
	{
		Hero->ddP += HeroMovementDirection;
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += 0.0f;
	}
	if(Input->MoveBack)
	{
		Hero->ddP -= HeroMovementDirection;
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += 180.0f;
	}
	if(Input->MoveLeft)
	{
		Hero->ddP -= Normalize(Cross(HeroMovementDirection, CameraUp));
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += 90.0f;
	}
	if(Input->MoveRight)
	{
		Hero->ddP += Normalize(Cross(HeroMovementDirection, CameraUp));
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += -90.0f;
	}
	if (Input->MoveForward && Input->MoveRight)
	{
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += -45.0f;
	}
	if (Input->MoveForward && Input->MoveLeft)
	{
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += 45.0f;
	}
	if (Input->MoveBack && Input->MoveRight)
	{
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += -135.0f;
	}
	if (Input->MoveBack && Input->MoveLeft)
	{
		Hero->Rot = Theta - 90.0f;
		Hero->Rot += 135.0f;
	}
	if (Input->MoveUp)
	{
		Hero->dY = 5.0f;
	}

	Hero->Attack = Input->MouseLeft;
	Hero->Fireball = Input->MouseRight;
	if(Input->MouseRight)
	{
		Input->MouseRight = false;
	}
	if (Input->MouseLeft)
	{
		Input->MouseLeft = false;
	}
}

int main(void)
{
	job_system_queue JobSystem;
	InitJobSystem(&JobSystem, 3);

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	uint32 Width = 900, Height = 540;

	GLFWwindow *Window = glfwCreateWindow(Width, Height, "VoxelEngine", 0, 0);
	game Game = {};
	glfwSetWindowUserPointer(Window, &Game.Input);
	glfwMakeContextCurrent(Window);
	glfwSwapInterval(0);
	glfwSetKeyCallback(Window, GLFWKeyCallback);
	glfwSetMouseButtonCallback(Window, GLFWMouseButtonCallback);
	glfwSetCursorPosCallback(Window, GLFWCursorPositionCallback);
	glViewport(0, 0, Width, Height);

	glewInit();

	GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *VidMode = glfwGetVideoMode(Monitor);
	// real32 GameUpdateHz = VidMode->refreshRate / 2.0f;
	int32 GameUpdateHz = VidMode->refreshRate;
	real32 TargetSecondsForFrame = 1.0f / GameUpdateHz;
	
	InitializeTextRenderer(&Game.TextRenderer);

	Game.JobSystem = &JobSystem;
	Game.WorldAllocatorSemaphore = CreateSemaphoreEx(0, 1, 1, 0, 0, SEMAPHORE_ALL_ACCESS);
	Game.Sound.InitAndLoadSounds();
	// Game.Sound.PlaySound2D(Sound_Music, true);

	void *MainMemory = malloc(Gigabytes(2));
	ZeroMemory(MainMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.WorldAllocator, Gigabytes(2), MainMemory);

	void *TransientMemory = malloc(Gigabytes(2));
	ZeroMemory(TransientMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.TranAllocator, Gigabytes(2), TransientMemory);

	Game.Assets = InitializeGameAssets(&Game.TranAllocator, Megabytes(64));

	world *World = &Game.World;
	InitializeWorld(World);

	Game.NullCollision = MakeNullCollision(&Game.WorldAllocator);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	camera *Camera = &Game.Camera;
	Camera->DistanceFromHero = 6.0f;
	Camera->Pitch = 0.0f;
	Camera->Head = 0.0f;
	Camera->RotSensetivity = 0.1f;
	Camera->OffsetFromHero = V3(0.0f, 0.0f, 5.0f);

	glClearColor(0.2549f, 0.4117f, 1.0f, 1.0f);
	glEnable(GL_MULTISAMPLE);
#if 1
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
#else
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
#endif
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	shader TestShader("shaders/vertex.vert", "shaders/fragment.frag");
	shader TestShader2D("shaders/2d.vert", "shaders/2d.frag");
	shader BlockParticleShader("shaders/block_particle.vert", "shaders/block_particle.frag");
	shader TextShader("shaders/text.vert", "shaders/text.frag");
	shader HUDShader("shaders/quad.vert", "shaders/quad.frag");
	shader HUD3DShader("shaders/3DHUD.vert", "shaders/3DHUD.frag");
	shader BillboardShader("shaders/billboard.vert", "shaders/billboard.frag");

	InitializeBlockParticleGenerator(&Game.BlockParticleGenerator, BlockParticleShader);

	real32 TestCrosshairVertices[] =
	{
		Width / 2.0f - 7.5f, Height / 2.0f + 7.5f + 70.0f, 0.0f, 1.0f,
		Width / 2.0f - 7.5f, Height / 2.0f - 7.5f + 70.0f, 0.0f, 0.0f,
		Width / 2.0f + 7.5f, Height / 2.0f + 7.5f + 70.0f, 1.0f, 1.0f,
		Width / 2.0f + 7.5f, Height / 2.0f - 7.5f + 70.0f, 1.0f, 0.0f,
	};
	GLuint VAOCrosshair, VBOCrosshair;
	glGenVertexArrays(1, &VAOCrosshair);
	glGenBuffers(1, &VBOCrosshair);
	glBindVertexArray(VAOCrosshair);
	glBindBuffer(GL_ARRAY_BUFFER, VBOCrosshair);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TestCrosshairVertices), TestCrosshairVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(real32), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(real32), (void *)(2*sizeof(real32)));
	glBindVertexArray(0);

	real32 QuadVertices[] = 
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
	};
	GLuint QuadVAO, QuadVBO;
	glGenVertexArrays(1, &QuadVAO);
	glGenBuffers(1, &QuadVBO);
	glBindVertexArray(QuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(real32), (void *)0);
	glBindVertexArray(0);

	GLuint CrosshairTexture = LoadTexture("data/textures/crosshair.tga");

	TestShader2D.Enable();
	mat4 Orthographic = Ortho(0.0f, (real32)Height, 0.0f, (real32)Width, -1.0f, 1.0f);
	TestShader2D.SetMat4("Projection", Orthographic);
	TestShader2D.SetInt("Texture", 0);

	HUDShader.Enable();
	HUDShader.SetMat4("Projection", Orthographic);

	TextShader.Enable();
	TextShader.SetMat4("Projection", Orthographic);
	TextShader.SetInt("Texture", 0);

	TestShader.Enable();
	mat4 PerspectiveMatrix = Perspective(45.0f, (real32)Width / (real32)Height, 0.1f, 100.0f);
	TestShader.SetMat4("Projection", PerspectiveMatrix);

	HUD3DShader.Enable();
	HUD3DShader.SetMat4("Projection", PerspectiveMatrix);

	BillboardShader.Enable();
	BillboardShader.SetMat4("Projection", PerspectiveMatrix);

	BlockParticleShader.Enable();
	BlockParticleShader.SetMat4("Projection", PerspectiveMatrix);

	// NOTE(georgy): Reserve entity slot 0
	AddLowEntity(&Game, EntityType_Null, InvalidPosition());

	world_position TestHeroPosition = {10004, 1, 10002, V3(0.0f, 0.25f, 0.0f)};
	Game.Hero = AddHero(&Game, TestHeroPosition);
	world_position TestTreePos = { 10004, 0, 10002, V3(4.0f, 3.3f, 0.0f) };
	AddTree(&Game, TestTreePos);
	world_position TestMonsterPos = { 10004, 0, 10002, V3(8.0f, 6.3f, 0.0f) };
	AddMonster(&Game, TestMonsterPos);

	real32 DeltaTime = TargetSecondsForFrame;
	real32 LastFrame = (real32)glfwGetTime();
	while (!glfwWindowShouldClose(Window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Game.Sound.Update(Game.Hero->Sim.FacingDir);
		ProcessInput(&Game.Input, Camera, &Game.HeroControl, DeltaTime);

		world_position Origin = Game.Hero->P;
		rect3 Bounds;
		Bounds.Min = V3(-60.0f, -30.0f, -60.0f);
		Bounds.Max = V3(60.0f, 60.0f, 60.0f);
		temporary_memory TempSimMemory = BeginTemporaryMemory(&Game.TranAllocator);
		sim_region *SimRegion = BeginSimulation(&Game.TranAllocator, &Game.WorldAllocator, World, Origin, Bounds);

		SetupChunks(Game.JobSystem, World, &Game.WorldAllocator, &Game.TranAllocator, Game.WorldAllocatorSemaphore);
		UpdateChunks(World);
		LoadChunks(World);

		UpdateCameraOffsetFromHero(Camera);

		TestShader.Enable();
		mat4 ViewRotation = RotationMatrixFromDirectionVector(Normalize(-Camera->OffsetFromHero));
		mat4 TranslationMatrix = Translation(-Camera->OffsetFromHero);
		mat4 ViewMatrix = ViewRotation * TranslationMatrix;
		RenderChunks(World, TestShader, &ViewMatrix, &PerspectiveMatrix);
		
		UpdateAndRenderEntities(SimRegion, &Game, BillboardShader, QuadVAO, DeltaTime, TestShader, &ViewMatrix, -Camera->OffsetFromHero);

		EndSimulation(SimRegion, World, &Game.WorldAllocator);
		EndTemporaryMemory(TempSimMemory);

		UpdateParticleGenerator(&Game.BlockParticleGenerator, DeltaTime);
		DrawParticles(&Game.BlockParticleGenerator, &ViewMatrix);

		TestShader2D.Enable();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(VAOCrosshair);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, CrosshairTexture);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glDisable(GL_BLEND);
		TestShader2D.Disable();

		HUDShader.Enable();
		glBindVertexArray(QuadVAO);
		mat4 HUDModel = Scale(V3(100.0f * ((real32)Game.Hero->Sim.HitPoints / Game.Hero->Sim.MaxHitPoints), 20.0f, 1.0f));
		HUDModel = Translation(V3(70.0f, Height - 50.0f, 0.0f)) * HUDModel;
		HUDShader.SetMat4("Model", HUDModel);
		HUDShader.SetVec4("Color", V4(1.0f, 0.0f, 0.0f, 1.0f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glEnable(GL_DEPTH_TEST);
		RenderTextNumber(&Game.TextRenderer, TextShader, Game.Hero->Sim.HitPoints, 100.0f, Height - 47.0f, 0.35f, V3(1.0f, 1.0f, 1.0f));

		if (Game.Assets->EntityModels[Asset_HeroHead])
		{
			HUD3DShader.Enable();
			glDisable(GL_DEPTH_TEST);

			mat4 Matrix = Scale(0.5f*Game.Assets->Infos[Asset_HeroHead].Scale);
			Matrix = Rotate(25.0f, V3(0.0f, 1.0f, 0.0f)) * Matrix;
			Matrix = Rotate(10.0f, V3(1.0f, 0.0f, 0.0f)) * Matrix;
			Matrix = Translation(V3(-1.9f, 1.05f, -3.0f)) * Matrix;
			HUD3DShader.SetMat4("View", Matrix);
			glBindVertexArray(Game.Assets->EntityModels[Asset_HeroHead]->VAO);
			glDrawArrays(GL_TRIANGLES, 0, Game.Assets->EntityModels[Asset_HeroHead]->VerticesCount);

			glEnable(GL_DEPTH_TEST);
			HUD3DShader.Disable();
		}

		DeltaTime = (real32)glfwGetTime() - LastFrame;
		// TODO(george): Implement sleeping instead of busy waiting
#if 1
		while (DeltaTime < TargetSecondsForFrame)
		{
			DeltaTime = (real32)glfwGetTime() - LastFrame;
		}
#endif
		if (DeltaTime > 1.0f)
		{
			DeltaTime = TargetSecondsForFrame;
		}
		LastFrame = (real32)glfwGetTime();

		RenderText(&Game.TextRenderer, TextShader, "FPS:", 0.0f, 15.0f, 0.5f, V3(1.0f, 0.0f, 0.0f));
		RenderTextNumber(&Game.TextRenderer, TextShader, (uint32)(1.0f / DeltaTime + 0.5f), 55.0f, 15.0f, 0.5f, V3(1.0f, 0.0f, 0.0f));

		glfwPollEvents();
		glfwSwapBuffers(Window);
	}

	glfwTerminate();

	return(0);
}