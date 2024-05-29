#pragma once

#include <glm/glm.hpp>
#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

#include <scene.h>
#include <shaderprogram.h>
#include <camera.h>

class Renderer
{
public:
	Scene* m_scene;
	Camera* m_camera;
	const ShaderProgram& m_program;

private:
	GLuint m_verticesTexture, m_indicesTexture;
	GLuint m_verticesBuffer,  m_indicesBuffer;

	GLuint m_uEye, m_uForward, m_uUp, m_uRight, m_uResolution;

public:
	Renderer(const ShaderProgram& program, Scene* scene, Camera* camera);
	~Renderer();

	void updateCamera() const;
};
