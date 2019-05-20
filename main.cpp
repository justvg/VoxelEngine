#include "engine.h"

global_variable game Game;

internal void 
GLFWKeyCallback(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods)
{
	if (Key == GLFW_KEY_W)
	{
		Game.Input.MoveForward = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_S)
	{
		Game.Input.MoveBack = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_A)
	{
		Game.Input.MoveLeft = (Action != GLFW_RELEASE);
	}
	if (Key == GLFW_KEY_D)
	{
		Game.Input.MoveRight = (Action != GLFW_RELEASE);
	}
}

internal void
GLFWMouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	if (Button == GLFW_MOUSE_BUTTON_1)
	{
		Game.Input.MouseLeft = (Action != GLFW_RELEASE);
	}
	if (Button == GLFW_MOUSE_BUTTON_2)
	{
		Game.Input.MouseRight = (Action != GLFW_RELEASE);
	}
}

internal void 
GLFWCursorPositionCallback(GLFWwindow* Window, double XPos, double YPos)
{
	Game.Input.MouseX = (int32)XPos;
	Game.Input.MouseY = (int32)YPos;
}

internal void
ProcessInput(game *Game, real32 DeltaTime)
{
	v3 CameraUp = V3(0.0f, 1.0f, 0.0f);

	real32 CameraSpeed = Game->Camera.MoveSpeed*DeltaTime;
	if (Game->Input.MoveForward)
	{
		Game->Camera.Position += Game->Camera.Front*CameraSpeed;
	}
	if (Game->Input.MoveBack)
	{
		Game->Camera.Position -= Game->Camera.Front*CameraSpeed;
	}
	if (Game->Input.MoveLeft)
	{
		Game->Camera.Position -= CameraSpeed*Normalize(Cross(Game->Camera.Front, CameraUp));
	}
	if (Game->Input.MoveRight)
	{
		Game->Camera.Position += CameraSpeed*Normalize(Cross(Game->Camera.Front, CameraUp));
	}

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

	float PitchRadians = Game->Camera.Pitch * (float)PI / 180.0f;
	float HeadRadians = Game->Camera.Head * (float)PI / 180.0f;
	Game->Camera.Front.x = cosf(PitchRadians)*cosf(HeadRadians);
	Game->Camera.Front.y = sinf(PitchRadians);
	Game->Camera.Front.z = cosf(PitchRadians)*sinf(HeadRadians);
	Game->Camera.Front.Normalize();
}

struct block
{
	bool32 Active; 
};

int main(void)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	uint32 Width = 900, Height = 500;

	GLFWwindow *Window = glfwCreateWindow(Width, Height, "VoxelEngine", 0, 0);
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

	Game.Sound.InitAndLoadSounds();
	// Game.Sound.PlaySound2D(sound_music);

	void *Memory = malloc(Gigabytes(2));
	InitializeStackAllocator(&Game.WorldAllocator, Gigabytes(1), Memory);

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Game.Camera.Position = V3(0.0f, 0.0f, 3.0f);
	Game.Camera.Front = V3(0.0f, 0.0f, -1.0f);
	Game.Camera.Pitch = 0.0f;
	Game.Camera.Head = -90.0f;
	Game.Camera.MoveSpeed = 3.0f;
	Game.Camera.RotSensetivity = 0.1f;

	glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_CULL_FACE);

	block *Chunk = PushArray(&Game.WorldAllocator, 16*16*16, block);

	shader TestShader("shaders/vertex.vert", "shaders/fragment.frag");

	real32 CubeVertices[] = {
		// Positions          
		-0.5f, -0.5f, -0.5f,  
		0.5f, -0.5f, -0.5f,  
		0.5f,  0.5f, -0.5f,  
		0.5f,  0.5f, -0.5f,  
		-0.5f,  0.5f, -0.5f,  
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

		0.5f,  0.5f,  0.5f,  
		0.5f,  0.5f, -0.5f,  
		0.5f, -0.5f, -0.5f,  
		0.5f, -0.5f, -0.5f,  
		0.5f, -0.5f,  0.5f,  
		0.5f,  0.5f,  0.5f,  

		-0.5f, -0.5f, -0.5f,  
		0.5f, -0.5f, -0.5f,  
		0.5f, -0.5f,  0.5f,  
		0.5f, -0.5f,  0.5f,  
		-0.5f, -0.5f,  0.5f,  
		-0.5f, -0.5f, -0.5f,  

		-0.5f,  0.5f, -0.5f,  
		0.5f,  0.5f, -0.5f,  
		0.5f,  0.5f,  0.5f,  
		0.5f,  0.5f,  0.5f,  
		-0.5f,  0.5f,  0.5f,  
		-0.5f,  0.5f, -0.5f
	};

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	glBindVertexArray(0);

	TestShader.Enable();
	mat4 Projection = Perspective(45.0f, (float)Width/(float)Height, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(TestShader.ID, "Projection"), 1, GL_FALSE, Projection.Elements);

	real32 DeltaTime = TargetSecondsForFrame;
	real32 LastFrame = (real32)glfwGetTime();
	while (!glfwWindowShouldClose(Window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// SoundEngine.Update(Camera.Position, Camera.Front);
		ProcessInput(&Game, DeltaTime);

		TestShader.Enable();
		glBindVertexArray(VAO);
		mat4 View = LookAt(Game.Camera.Position, Game.Camera.Position + Game.Camera.Front);
		glUniformMatrix4fv(glGetUniformLocation(TestShader.ID, "View"), 1, GL_FALSE, View.Elements);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		DeltaTime = (real32)glfwGetTime() - LastFrame;
		// TODO(george): Implement sleeping instead of busy waiting
#if 1
		while(DeltaTime < TargetSecondsForFrame)
		{
			DeltaTime = (real32)glfwGetTime() - LastFrame;
		}
#endif
		LastFrame = (real32)glfwGetTime();

		std::cout << DeltaTime << std::endl;

		glfwPollEvents();
		glfwSwapBuffers(Window);
	}

	glfwTerminate();

	return(0);
}