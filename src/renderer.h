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
	const ShaderProgram& m_postProgram;

private:
	GLuint m_fbo;
	GLuint m_accumulationTexture;

	GLuint m_verticesTexture, m_indicesTexture, m_vertexDataTexture, m_lightTexture;
	GLuint m_verticesBuffer,  m_indicesBuffer,  m_vertexDataBuffer,  m_lightBuffer;

	GLuint m_uEye, m_uForward, m_uUp, m_uRight, m_uResolution;

	uint m_iterationCount;

public:
	Renderer(const ShaderProgram& program, const ShaderProgram& postProgram, Scene* scene, Camera* camera);
	~Renderer();

	void draw();
	void reset();
	void resize(const glm::uvec2& resolution);
	void updateCamera();
};
