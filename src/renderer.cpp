#include <renderer.h>

#include <iostream>

Renderer::Renderer(Scene* scene, glm::ivec2 resolution, ShaderProgram& program)
	: m_scene(scene), m_resolution(resolution)
{
	glActiveTexture(GL_TEXTURE0);
	glGenBuffers(1, &m_verticesBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_verticesBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec3) * scene->m_vertices.size(), &scene->m_vertices[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_verticesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_verticesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_verticesBuffer);

	glActiveTexture(GL_TEXTURE1);
	glGenBuffers(1, &m_indicesBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_indicesBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec3) * scene->m_indices.size(), &scene->m_indices[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_indicesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_indicesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_indicesBuffer);

	glUseProgram(program.m_id);
	glUniform1i(glGetUniformLocation(program.m_id, "verticesTex"), 0);
    glUniform1i(glGetUniformLocation(program.m_id, "indicesTex"), 1);
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_verticesTexture);
	glDeleteTextures(1, &m_indicesBuffer);

	glDeleteBuffers(1, &m_verticesBuffer);
	glDeleteBuffers(1, &m_indicesBuffer);
}
