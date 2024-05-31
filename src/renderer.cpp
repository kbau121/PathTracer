#include <renderer.h>

Renderer::Renderer(const ShaderProgram& program, Scene* scene, Camera* camera)
	: m_scene(scene), m_camera(camera), m_program(program)
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

	m_uEye = glGetUniformLocation(program.m_id, "eye");
    m_uForward = glGetUniformLocation(program.m_id, "forward");
    m_uUp = glGetUniformLocation(program.m_id, "up");
    m_uRight = glGetUniformLocation(program.m_id, "right");
    m_uResolution = glGetUniformLocation(program.m_id, "resolution");

	glUseProgram(program.m_id);
	glUniform1ui(glGetUniformLocation(program.m_id, "indexCount"), scene->m_indices.size());
	glUniform1i(glGetUniformLocation(program.m_id, "verticesTex"), 0);
    glUniform1i(glGetUniformLocation(program.m_id, "indicesTex"), 1);
    glUseProgram(0);

    updateCamera();
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_verticesTexture);
	glDeleteTextures(1, &m_indicesBuffer);

	glDeleteBuffers(1, &m_verticesBuffer);
	glDeleteBuffers(1, &m_indicesBuffer);
}

void Renderer::updateCamera() const
{
	m_camera->update();

	glUseProgram(m_program.m_id);
    glUniform3fv(m_uEye, 1, &m_camera->m_eye[0]);
    glUniform3fv(m_uForward, 1, &m_camera->m_forward[0]);
    glUniform3fv(m_uUp, 1, &m_camera->m_up[0]);
    glUniform3fv(m_uRight, 1, &m_camera->m_right[0]);
    glUniform2uiv(m_uResolution, 1, &m_camera->m_resolution[0]);
    glUseProgram(0);
}
