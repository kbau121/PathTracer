#pragma once

#include <glm/glm.hpp>

#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

#include <vector>

class Scene
{
public:
	std::vector<glm::vec3> m_vertices;
	std::vector<glm::vec3> m_indices;

	Scene()
		: m_vertices(std::vector<glm::vec3>
			{glm::vec3(-1.f, -1.f, 5.f), glm::vec3(1.f, -1.f, 5.f), glm::vec3(0.f, 1.f, 5.f)}
			),
		  m_indices(std::vector<glm::vec3> {glm::vec3(0, 1, 2)})
	{ }
};
