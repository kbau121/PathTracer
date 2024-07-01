#include <renderer.h>

#define VERTICES_TEXTURE     GL_TEXTURE0
#define INDICES_TEXTURE      GL_TEXTURE1
#define VERTEX_DATA_TEXTURE  GL_TEXTURE2
#define ACCUMULATION_TEXTURE GL_TEXTURE3
#define LIGHT_TEXTURE        GL_TEXTURE4
#define MATERIAL_TEXTURE     GL_TEXTURE5
#define MATERIAL_MAP_TEXTURE GL_TEXTURE6

Renderer::Renderer(const ShaderProgram& program, const ShaderProgram& postProgram, Scene* scene, Camera* camera)
	: m_scene(scene), m_camera(camera), m_program(program), m_postProgram(postProgram), m_iterationCount(0)
{
	// Framebuffer
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glActiveTexture(ACCUMULATION_TEXTURE);
	glGenTextures(1, &m_accumulationTexture);
	glBindTexture(GL_TEXTURE_2D, m_accumulationTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, camera->m_resolution.x, camera->m_resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_accumulationTexture, 0);

	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Vertices
	glActiveTexture(VERTICES_TEXTURE);
	glGenBuffers(1, &m_verticesBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_verticesBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec3) * scene->m_vertices.size(), &scene->m_vertices[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_verticesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_verticesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_verticesBuffer);

	// Indices
	glActiveTexture(INDICES_TEXTURE);
	glGenBuffers(1, &m_indicesBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_indicesBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec4) * scene->m_indices.size(), &scene->m_indices[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_indicesTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_indicesTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, m_indicesBuffer);

	// Vertex Data
	glActiveTexture(VERTEX_DATA_TEXTURE);
	glGenBuffers(1, &m_vertexDataBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_vertexDataBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(Scene::VertexData) * scene->m_vertexData.size(), &scene->m_vertexData[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_vertexDataTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_vertexDataTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_vertexDataBuffer);

	// Lights
	glActiveTexture(LIGHT_TEXTURE);
	glGenBuffers(1, &m_lightBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_lightBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(Scene::Light) * scene->m_lights.size(), &scene->m_lights[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_lightTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_lightTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_lightBuffer);

	// Materials
	glActiveTexture(MATERIAL_TEXTURE);
	glGenBuffers(1, &m_materialBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_materialBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(Scene::Material) * scene->m_materials.size(), &scene->m_materials[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_materialTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_materialTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, m_materialBuffer);

	glActiveTexture(MATERIAL_MAP_TEXTURE);
	glGenBuffers(1, &m_materialMapBuffer);
	glBindBuffer(GL_TEXTURE_BUFFER, m_materialMapBuffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(uint32_t) * scene->m_materialMap.size(), &scene->m_materialMap[0], GL_STATIC_DRAW);
	glGenTextures(1, &m_materialMapTexture);
	glBindTexture(GL_TEXTURE_BUFFER, m_materialMapTexture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_materialMapBuffer);

	// Uniforms
	m_uEye = glGetUniformLocation(program.m_id, "eye");
    m_uForward = glGetUniformLocation(program.m_id, "forward");
    m_uUp = glGetUniformLocation(program.m_id, "up");
    m_uRight = glGetUniformLocation(program.m_id, "right");
    m_uResolution = glGetUniformLocation(program.m_id, "resolution");

	glUseProgram(program.m_id);
	glUniform1ui(glGetUniformLocation(program.m_id, "indexCount"), scene->m_indices.size());
	glUniform1i(glGetUniformLocation(program.m_id, "verticesTex"), VERTICES_TEXTURE - GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(program.m_id, "indicesTex"), INDICES_TEXTURE - GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(program.m_id, "vertexDataTex"), VERTEX_DATA_TEXTURE - GL_TEXTURE0);

    glUniform1i(glGetUniformLocation(program.m_id, "accumTexture"), ACCUMULATION_TEXTURE - GL_TEXTURE0);

    glUniform1i(glGetUniformLocation(program.m_id, "lightTex"), LIGHT_TEXTURE - GL_TEXTURE0);
    glUniform4uiv(glGetUniformLocation(program.m_id, "lightCount"), 1, &scene->m_lightCount[0]);

    glUniform1i(glGetUniformLocation(program.m_id, "materialTex"), MATERIAL_TEXTURE - GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(program.m_id, "materialMapTex"), MATERIAL_MAP_TEXTURE - GL_TEXTURE0);

    glUseProgram(m_postProgram.m_id);
    glUniform1i(glGetUniformLocation(postProgram.m_id, "inTexture"), ACCUMULATION_TEXTURE - GL_TEXTURE0);

    glUseProgram(0);

    updateCamera();
}

Renderer::~Renderer()
{
	glDeleteTextures(1, &m_verticesTexture);
	glDeleteTextures(1, &m_indicesTexture);
	glDeleteTextures(1, &m_accumulationTexture);

	glDeleteBuffers(1, &m_verticesBuffer);
	glDeleteBuffers(1, &m_indicesBuffer);
}

void Renderer::draw()
{
	// Accumulation

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glUseProgram(m_program.m_id);

	glUniform1ui(glGetUniformLocation(m_program.m_id, "iterationCount"), m_iterationCount);
	m_iterationCount++;

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Output

	glViewport(0, 0, m_camera->m_resolution.x, m_camera->m_resolution.y);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(m_postProgram.m_id);
	glActiveTexture(ACCUMULATION_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, m_accumulationTexture);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
}

void Renderer::reset()
{
	m_iterationCount = 0;

	glViewport(0, 0, m_camera->m_resolution.x, m_camera->m_resolution.y);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::resize(const glm::uvec2& resolution)
{
	m_camera->m_resolution = resolution;

	// Recreate the accumulation texture
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glActiveTexture(ACCUMULATION_TEXTURE);
	glGenTextures(1, &m_accumulationTexture);
	glBindTexture(GL_TEXTURE_2D, m_accumulationTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_accumulationTexture, 0);

	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Update the shader's stored resolution
	glUseProgram(m_program.m_id);
	glUniform2uiv(m_uResolution, 1, &m_camera->m_resolution[0]);
	glUseProgram(0);

	reset();
}

void Renderer::updateCamera()
{
	m_camera->update();

	glUseProgram(m_program.m_id);
    glUniform3fv(m_uEye, 1, &m_camera->m_eye[0]);
    glUniform3fv(m_uForward, 1, &m_camera->m_forward[0]);
    glUniform3fv(m_uUp, 1, &m_camera->m_up[0]);
    glUniform3fv(m_uRight, 1, &m_camera->m_right[0]);
    glUniform2uiv(m_uResolution, 1, &m_camera->m_resolution[0]);
    glUseProgram(0);

    reset();
}
