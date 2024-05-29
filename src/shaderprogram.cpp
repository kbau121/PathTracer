#include <shaderprogram.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

/// Logs any errors found when compiling a shader
/// Returns true if no errors were found, otherwise returns false
bool ShaderProgram::logCompileErrors(GLuint shader) const
{
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        GLchar* errorLog = new GLchar[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);

        std::cout << errorLog << std::endl;

        return false;
    }

    return true;
}

/// Reads a file into a new character array
char* ShaderProgram::readfile(const char* filename) const
{
	FILE* file = fopen(filename, "r");
	if (!file) return nullptr;
	DEFER(fclose(file));

	// File size
	if (fseeko(file, 0, SEEK_END) != 0) return nullptr;
	size_t fileSize = ftello(file);
	if (fseeko(file, 0, SEEK_SET) != 0) return nullptr;

	// Read the contents
	char* contents = new char[fileSize + 1];
	size_t readSize = fread(contents, 1, fileSize, file);
	UNUSED(readSize);
	contents[fileSize] = '\0';

	return contents;
}

/// Creates a new GL program given the specified vertex/fragment shader file locations
ShaderProgram::ShaderProgram(const char* vertFile, const char* fragFile)
	: m_id(-1u), m_isCompiled(false)
{
	GLuint vertShader, fragShader;

	m_id = glCreateProgram();

	// Read the shader files
	const char* vertShaderCode = readfile(vertFile);
	DEFER(delete vertShaderCode);
	const char* fragShaderCode = readfile(fragFile);
	DEFER(delete fragShaderCode);

	// Create the vertex shader
	vertShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertShader, 1, &vertShaderCode, nullptr);
	glCompileShader(vertShader);
	DEFER(glDeleteShader(vertShader));

	if (!logCompileErrors(vertShader))
	{
		return;
	}

	// Create the fragment shader
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragShader, 1, &fragShaderCode, nullptr);
	glCompileShader(fragShader);
	DEFER(glDeleteShader(fragShader));

	if (!logCompileErrors(fragShader))
	{
		return;
	}

	// Link the program and shaders
	glAttachShader(m_id, vertShader);
	glAttachShader(m_id, fragShader);
	glLinkProgram(m_id);

	m_isCompiled = true;
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(m_id);
}

/// Checks if the program has succesfully compiled
bool ShaderProgram::isCompiled() const
{
	return m_isCompiled;
}