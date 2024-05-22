#include <renderer.h>

#include <iostream>

Renderer::Renderer(ShaderProgram& program, Scene* scene, glm::uvec2 resolution)
	: m_scene(scene), m_program(program), m_eyePos(0.f, 0.f, 0.f), m_eyeDir(0.f, 0.f, 1.f)
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

	m_uEyePos = glGetUniformLocation(program.m_id, "eyePos");
    m_uEyeDir = glGetUniformLocation(program.m_id, "eyeDir");
    m_uResolution = glGetUniformLocation(program.m_id, "resolution");

	glUseProgram(program.m_id);
	glUniform1i(glGetUniformLocation(program.m_id, "verticesTex"), 0);
    glUniform1i(glGetUniformLocation(program.m_id, "indicesTex"), 1);
    glUniform3fv(m_uEyePos, 1, &m_eyePos[0]);
    glUniform3fv(m_uEyeDir, 1, &m_eyeDir[0]);
    glUniform2uiv(m_uResolution, 1, &resolution[0]);
    glUseProgram(0);
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_verticesTexture);
	glDeleteTextures(1, &m_indicesBuffer);

	glDeleteBuffers(1, &m_verticesBuffer);
	glDeleteBuffers(1, &m_indicesBuffer);
}

void Renderer::setCamera(glm::vec3 eyePos, glm::vec3 eyeDir, glm::uvec2 resolution)
{
	m_eyePos = eyePos;
	m_eyeDir = eyeDir;

	glUseProgram(m_program.m_id);
    glUniform3fv(m_uEyePos, 1, &m_eyePos[0]);
    glUniform3fv(m_uEyeDir, 1, &m_eyeDir[0]);
    glUniform2uiv(m_uResolution, 1, &resolution[0]);
    glUseProgram(0);
}
