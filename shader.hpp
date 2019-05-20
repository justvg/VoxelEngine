#pragma once

struct shader
{
	GLuint ID;
	shader(const char *VertexPath, const char *FragmentPath, const char *GeometryPath = 0);

	void Enable();
	void Disable();
};

internal char *
ReadEntireFile(const char *Filename)
{
	FILE *File = fopen(Filename, "rb");
	fseek(File, 0, SEEK_END);
	uint32 FileSize = ftell(File);
	fseek(File, 0, SEEK_SET);

	char *String = (char *)malloc(FileSize + 1);
	fread(String, 1, FileSize, File);
	fclose(File);

	String[FileSize] = 0;

	return(String);
}

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