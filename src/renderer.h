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
	glm::ivec2 m_resolution;

	GLuint m_verticesTexture, m_indicesTexture;
	GLuint m_verticesBuffer,  m_indicesBuffer;

public:
	Renderer(Scene* scene, glm::ivec2 resolution, ShaderProgram& program);
	~Renderer();
};
