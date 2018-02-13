#pragma once
// Author:  George Othen
// Date: 14/02/2017
// Title: Shader Class for Opening Shaders

// Source: https://learnopengl.com
// Credit: JOEY DE VRIES

// GL Includes
#include <GL/glew.h>

class Shader
{
public:
	GLuint Program;

	Shader(const GLchar * vertexPath, const GLchar * fragmentPath);

	void Use();
};

