#pragma once

struct shader
{
	GLuint ID;

	shader(){};
	shader(const char *VertexPath, const char *FragmentPath, const char *GeometryPath = 0);

	void Enable();
	void Disable();
	void SetReal(char *Name, real32 Value);
	void SetInt(char *Name, int32 Value);
	void SetVec2(char *Name, v2 &Vector);
	void SetVec3(char *Name, v3 &Vector);
	void SetVec4(char *Name, v4 &Vector);
	void SetMat4(char *Name, mat4 &Mat);
};

shader::shader(const char *VertexPath, const char *FragmentPath, const char *GeometryPath)
{
	GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	ID = glCreateProgram();

	const char *VertexSourceCode = ReadEntireFile(VertexPath);
	const char *FragmentSourceCode = ReadEntireFile(FragmentPath);

	glShaderSource(VertexShader, 1, &VertexSourceCode, 0);
	glCompileShader(VertexShader);
	int Success;
	char InfoLog[1024];
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(VertexShader, 1024, NULL, InfoLog);
		std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << "VertexShader" << "\n" << InfoLog << "\n -- --------------------------------------------------- -- " << std::endl;
	}

	if (GeometryPath)
	{
		const char *GeometrySourceCode = ReadEntireFile(GeometryPath);

		glShaderSource(GeometryShader, 1, &GeometrySourceCode, 0);
		glCompileShader(GeometryShader);
		glGetShaderiv(GeometryShader, GL_COMPILE_STATUS, &Success);
		if (!Success)
		{
			glGetShaderInfoLog(GeometryShader, 1024, NULL, InfoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << "GeometryShader" << "\n" << InfoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}

	glShaderSource(FragmentShader, 1, &FragmentSourceCode, 0);
	glCompileShader(FragmentShader);
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(FragmentShader, 1024, NULL, InfoLog);
		std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << "FragmentShader" << "\n" << InfoLog << "\n -- --------------------------------------------------- -- " << std::endl;
	}

	glAttachShader(ID, VertexShader);
	glAttachShader(ID, FragmentShader);
	if (GeometryPath)
	{
		glAttachShader(ID, GeometryShader);
	}
	glLinkProgram(ID);
	if (!Success)
	{
		glGetProgramInfoLog(ID, 1024, NULL, InfoLog);
		std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << "Program" << "\n" << InfoLog << "\n -- --------------------------------------------------- -- " << std::endl;
	}

	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
	glDeleteShader(GeometryShader);
}

void shader::Enable()
{
	glUseProgram(ID);
}

void shader::Disable()
{
	glUseProgram(0);
}

void shader::SetReal(char *Name, real32 Value)
{
	glUniform1f(glGetUniformLocation(ID, Name), Value);
}

void shader::SetInt(char *Name, int32 Value)
{
	glUniform1i(glGetUniformLocation(ID, Name), Value);
}

void shader::SetVec2(char *Name, v2 &Vector)
{
	glUniform2f(glGetUniformLocation(ID, Name), Vector.x, Vector.y);
}

void shader::SetVec3(char *Name, v3 &Vector)
{
	glUniform3f(glGetUniformLocation(ID, Name), Vector.x, Vector.y, Vector.z);
}

void shader::SetVec4(char *Name, v4 &Vector)
{
	glUniform4f(glGetUniformLocation(ID, Name), Vector.x, Vector.y, Vector.z, Vector.w);
}

void shader::SetMat4(char *Name, mat4 &Mat)
{
	glUniformMatrix4fv(glGetUniformLocation(ID, Name), 1, GL_FALSE, Mat.Elements);
}