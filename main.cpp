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
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	uint32 Width = 900, Height = 540;

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

	void *MainMemory = malloc(Gigabytes(2));
	ZeroMemory(MainMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.WorldAllocator, Gigabytes(2), MainMemory);

	void *TransientMemory = malloc(Gigabytes(2));
	ZeroMemory(TransientMemory, Gigabytes(2));
	InitializeStackAllocator(&Game.TranAllocator, Gigabytes(2), TransientMemory);

	Game.World.ChunkDimInMeters = 8.0f;
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

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
#if 1
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
#else
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
#endif
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	shader GBufferShader("shaders/gbuffervertex.vert", "shaders/gbufferfragment.frag");
	shader DebugQuadShader("shaders/debugquad.vert", "shaders/debugquad.frag");
	shader SSAOShader("shaders/ssao.vert", "shaders/ssao.frag");
	shader SSAOBlurShader("shaders/ssao.vert", "shaders/ssaoblur.frag");

	real32 QuadVertices[] =
	{
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f
	};

	GLuint QuadVAO, QuadVBO;
	glGenVertexArrays(1, &QuadVAO);
	glGenBuffers(1, &QuadVBO);
	glBindVertexArray(QuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), QuadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(real32), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(real32), (void *)(3*sizeof(real32)));
	glBindVertexArray(0);

	GLuint GBufferFBO;
	glGenFramebuffers(1, &GBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, GBufferFBO);
	
	GLuint GBufferPos, GBufferNormals, GBufferAlbedo;
	glGenTextures(1, &GBufferPos);
	glBindTexture(GL_TEXTURE_2D, GBufferPos);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Width, Height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GBufferPos, 0);

	glGenTextures(1, &GBufferNormals);
	glBindTexture(GL_TEXTURE_2D, GBufferNormals);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Width, Height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GBufferNormals, 0);

	glGenTextures(1, &GBufferAlbedo);
	glBindTexture(GL_TEXTURE_2D, GBufferAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GBufferAlbedo, 0);

	GLuint GBufferDepthRBO;
	glGenRenderbuffers(1, &GBufferDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, GBufferDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GBufferDepthRBO);

	GLuint GBufferFBOAttachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, GBufferFBOAttachments);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "GBufferFBO isn't complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint SSAOFBO, SSAOOcclusionBuffer;
	glGenFramebuffers(1, &SSAOFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOFBO);

	glGenTextures(1, &SSAOOcclusionBuffer);
	glBindTexture(GL_TEXTURE_2D, SSAOOcclusionBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width, Height, 0, GL_RED, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOOcclusionBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "SSAOFBO isn't complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint SSAOBlurFBO, SSAOBlurOcclusions;
	glGenFramebuffers(1, &SSAOBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOBlurFBO);

	glGenTextures(1, &SSAOBlurOcclusions);
	glBindTexture(GL_TEXTURE_2D, SSAOBlurOcclusions);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, Width, Height, 0, GL_RED, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOBlurOcclusions, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "SSAOBlurFBO isn't complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	srand(1337);
	v3 SSAOSampleKernel[16];
	for (uint32 I = 0; I < 16; I++)
	{
		v3 Sample;
		Sample.x = ((real32)rand() / (real32)RAND_MAX) * 2.0f - 1.0f;
		Sample.y = ((real32)rand() / (real32)RAND_MAX) * 2.0f - 1.0f;
		Sample.z = ((real32)rand() / (real32)RAND_MAX);
		Sample.Normalize();

		real32 Scale = I / 16.0f;
		Scale = Lerp(0.1f, 1.0f, Scale*Scale);
		Sample = Sample * Scale;
		
		SSAOSampleKernel[I] = Sample;
	}

	v3 SSAONoiseRotation[16];
	for (uint32 I = 0; I < 16; I++)
	{
		v3 NoiseRotation;
		NoiseRotation.x = ((real32)rand() / (real32)RAND_MAX) * 2.0f - 1.0f;
		NoiseRotation.y = ((real32)rand() / (real32)RAND_MAX) * 2.0f - 1.0f;
		NoiseRotation.z = 0.0f;
		NoiseRotation.Normalize();

		SSAONoiseRotation[I] = NoiseRotation;
	}
	GLuint NoiseTexture;
	glGenTextures(1, &NoiseTexture);
	glBindTexture(GL_TEXTURE_2D, NoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, SSAONoiseRotation);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GBufferShader.Enable();
	mat4 Projection = Perspective(45.0f, (float)Width/(float)Height, 0.1f, 100.0f);
	glUniformMatrix4fv(glGetUniformLocation(GBufferShader.ID, "Projection"), 1, GL_FALSE, Projection.Elements);

	SSAOShader.Enable();
	glUniform1i(glGetUniformLocation(SSAOShader.ID, "gPos"), 0);
	glUniform1i(glGetUniformLocation(SSAOShader.ID, "gNormal"), 1);
	glUniform1i(glGetUniformLocation(SSAOShader.ID, "NoiseRotation"), 2);
	glUniform3fv(glGetUniformLocation(SSAOShader.ID, "SampleKernel"), 16, (const GLfloat *)SSAOSampleKernel);
	glUniformMatrix4fv(glGetUniformLocation(SSAOShader.ID, "Projection"), 1, GL_FALSE, Projection.Elements);

	SSAOBlurShader.Enable();
	glUniform1i(glGetUniformLocation(SSAOBlurShader.ID, "Occlusions"), 0);

	DebugQuadShader.Enable();
	glUniform1i(glGetUniformLocation(DebugQuadShader.ID, "DebugTextureColors"), 0);
	glUniform1i(glGetUniformLocation(DebugQuadShader.ID, "SSAO"), 1);

	real32 DeltaTime = TargetSecondsForFrame;
	real32 LastFrame = (real32)glfwGetTime();
	while (!glfwWindowShouldClose(Window))
	{
		// SoundEngine.Update(Camera->Position, Camera->Front);
		ProcessInput(&Game, DeltaTime);

		world_position Origin = Camera->Position;
		rect3 Bounds;
		Bounds.Min = V3(-40.0f, -5.0f, -40.0f);
		Bounds.Max = V3(40.0f, 5.0f, 40.0f);
		temporary_memory TempSimMemory = BeginTemporaryMemory(&Game.TranAllocator);
		BeginSimulation(&Game.TranAllocator, &Game.WorldAllocator, &Game.World, Origin, Bounds);

		glBindFramebuffer(GL_FRAMEBUFFER, GBufferFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GBufferShader.Enable();
		mat4 ViewRotation = RotationMatrixFromDirectionVector(Camera->Front);
		RenderChunks(&Game.World, GBufferShader.ID, &ViewRotation);

		glBindFramebuffer(GL_FRAMEBUFFER, SSAOFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		SSAOShader.Enable();
		glBindVertexArray(QuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GBufferPos);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, GBufferNormals);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, NoiseTexture);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, SSAOBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		SSAOBlurShader.Enable();
		glBindVertexArray(QuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, SSAOOcclusionBuffer);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		DebugQuadShader.Enable();
		glBindVertexArray(QuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GBufferAlbedo);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, SSAOBlurOcclusions);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		EndSimulation(&Game.World);
		EndTemporaryMemory(TempSimMemory);

		DeltaTime = (real32)glfwGetTime() - LastFrame;
		// TODO(george): Implement sleeping instead of busy waiting
#if 1
		while(DeltaTime < TargetSecondsForFrame)
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