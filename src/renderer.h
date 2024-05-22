#pragma once

#include <glm/glm.hpp>
#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

#include <scene.h>
#include <shaderprogram.h>

class Renderer
{
public:
	Scene* m_scene;
	ShaderProgram& m_program;

private:
	GLuint m_verticesTexture, m_indicesTexture;
	GLuint m_verticesBuffer,  m_indicesBuffer;

	GLuint m_uEyePos, m_uEyeDir, m_uResolution;

	glm::vec3 m_eyePos, m_eyeDir;

public:
	Renderer(ShaderProgram& program, Scene* scene, glm::uvec2 resolution);
	~Renderer();

	void setCamera(glm::vec3 eyePos, glm::vec3 eyeDir, glm::uvec2 resolution);
};
