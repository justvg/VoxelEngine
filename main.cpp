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
		Input->MouseRight = (Action != GLFW_RELEASE);
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
ProcessInput(game *Game, real32 DeltaTime)
{
	v3 CameraUp = V3(0.0f, 1.0f, 0.0f);

	real32 CameraSpeed = Game->Camera.MoveSpeed*DeltaTime;
	if (Game->Input.MoveForward)
	{
		Game->Camera.Position.Offset += Game->Camera.Front*CameraSpeed;
	}
	if (Game->Input.MoveBack)
	{
		Game->Camera.Position.Offset -= Game->Camera.Front*CameraSpeed;
	}
	if (Game->Input.MoveLeft)
	{
		Game->Camera.Position.Offset -= CameraSpeed*Normalize(Cross(Game->Camera.Front, CameraUp));
	}
	if (Game->Input.MoveRight)
	{
		Game->Camera.Position.Offset += CameraSpeed*Normalize(Cross(Game->Camera.Front, CameraUp));
	}
	RecanonicalizeCoords(&Game->World, &Game->Camera.Position);

	int32 X, Y;
	X = Game->Input.MouseX;
	Y = Game->Input.MouseY;
	static int32 LastMouseX = 0;
	static int32 LastMouseY = 0;
	static bool FirstMouse = true;
	if (FirstMouse)
	{
		LastMouseX = X;
		LastMouseY = Y;
		FirstMouse = false;
	}

	real32 XOffset = (real32)((X - LastMouseX)*Game->Camera.RotSensetivity);
	real32 YOffset = (real32)((LastMouseY - Y)*Game->Camera.RotSensetivity);

	Game->Camera.Pitch += YOffset;
	Game->Camera.Head += XOffset;

	Game->Camera.Pitch = Game->Camera.Pitch > 89.0f ? 89.0f : Game->Camera.Pitch;
	Game->Camera.Pitch = Game->Camera.Pitch < -89.0f ? -89.0f : Game->Camera.Pitch;

	LastMouseX = X;
	LastMouseY = Y;

	float PitchRadians = Game->Camera.Pitch * (float)M_PI / 180.0f;
	float HeadRadians = Game->Camera.Head * (float)M_PI / 180.0f;
	Game->Camera.Front.x = cosf(PitchRadians)*cosf(HeadRadians);
	Game->Camera.Front.y = sinf(PitchRadians);
	Game->Camera.Front.z = cosf(PitchRadians)*sinf(HeadRadians);
	Game->Camera.Front.Normalize();
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

	Game.JobSystem = &JobSystem;
	Game.WorldAllocatorSemaphore = CreateSemaphoreEx(0, 1, 1, 0, 0, SEMAPHORE_ALL_ACCESS);
	// Game.Sound.InitAndLoadSounds();
	// Game.Sound.PlaySound2D(sound_music);

	void *MainMemory = malloc(Gigabytes(2));
	ZeroMemory(MainMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.WorldAllocator, Gigabytes(2), MainMemory);

	void *TransientMemory = malloc(Gigabytes(2));
	ZeroMemory(TransientMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.TranAllocator, Gigabytes(2), TransientMemory);

	InitRaySample(&Game.World);
	Game.World.ChunkDimInMeters = CHUNK_DIM / 2.0f;
	Game.World.BlockDimInMeters = Game.World.ChunkDimInMeters / CHUNK_DIM;
	InitBiomes(&Game.World);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	camera *Camera = &Game.Camera;
	Camera->Position.ChunkX = 10000;
	Camera->Position.ChunkY = 0;
	Camera->Position.ChunkZ = 10000;
	Camera->Position.Offset = V3(0.0f, 0.0f, 0.0f);
	Camera->Front = V3(0.0f, 0.0f, -1.0f);
	Camera->Pitch = 0.0f;
	Camera->Head = -90.0f;
	// Camera->MoveSpeed = 3.0f;
	Camera->MoveSpeed = 8.0f;
	Camera->RotSensetivity = 0.1f;

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

	TestShader.Enable();
	mat4 Projection = Perspective(45.0f, (real32)Width / (real32)Height, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(TestShader.ID, "Projection"), 1, GL_FALSE, Projection.Elements);

	real32 DeltaTime = TargetSecondsForFrame;
	real32 LastFrame = (real32)glfwGetTime();
	while (!glfwWindowShouldClose(Window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// SoundEngine.Update(Camera->Position, Camera->Front);
		ProcessInput(&Game, DeltaTime);
		world_position Origin = Camera->Position;
		rect3 Bounds;
		Bounds.Min = V3(-70.0f, -2.0f, -70.0f);
		Bounds.Max = V3(70.0f, 15.0f, 70.0f);
		temporary_memory TempSimMemory = BeginTemporaryMemory(&Game.TranAllocator);
		BeginSimulation(&Game.TranAllocator, &Game.WorldAllocator, &Game.World, Origin, Bounds);

		SetupChunks(Game.JobSystem, &Game.World, &Game.WorldAllocator, &Game.TranAllocator, Game.WorldAllocatorSemaphore);
		LoadChunks(&Game.World);

		TestShader.Enable();
		mat4 ViewRotation = RotationMatrixFromDirectionVector(Camera->Front);
		RenderChunks(&Game.World, TestShader.ID, &ViewRotation, &Projection);

		EndSimulation(&Game.World);
		EndTemporaryMemory(TempSimMemory);

		// std::cout << Camera->Position.ChunkX << " " << Camera->Position.ChunkY << " " << Camera->Position.ChunkZ << std::endl;

		DeltaTime = (real32)glfwGetTime() - LastFrame;
		// TODO(george): Implement sleeping instead of busy waiting
#if 0
		while (DeltaTime < TargetSecondsForFrame)
		{
			DeltaTime = (real32)glfwGetTime() - LastFrame;
		}
#endif
		LastFrame = (real32)glfwGetTime();

		// std::cout << DeltaTime << std::endl;

		glfwPollEvents();
		glfwSwapBuffers(Window);
	}

	glfwTerminate();

	return(0);
}