#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

internal GLuint
LoadTexture(const char *Filename)
{
	GLuint TextureID;
	glGenTextures(1, &TextureID);

	int32 Width, Height, nChannels;
	unsigned char *Data = stbi_load(Filename, &Width, &Height, &nChannels, 0);
	if (Data)
	{
		GLenum Format;
		if(nChannels == 1) Format = GL_RED;
		else if(nChannels == 3) Format = GL_RGB;
		else if(nChannels == 4) Format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, TextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		std::cout << "Can't load texture " << Filename << std::endl;
	}
	stbi_image_free(Data);

	return(TextureID);
}

