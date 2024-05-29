#pragma once

#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

class ShaderProgram
{
public:
	GLuint m_id;

private:
	bool m_isCompiled;

	bool logCompileErrors(GLuint shader) const;
	char* readfile(const char* filename) const;

public:
	ShaderProgram(const char* vertFile, const char* fragFile);
	~ShaderProgram();

	bool isCompiled() const;
};
