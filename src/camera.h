#pragma once

#include <glm/glm.hpp>

#include <error_handling.h>
#include <gl/gl.h>
#include <utils.h>

class Camera
{
public:
	glm::vec3 m_eye;
	glm::vec3 m_focus;

	glm::vec3 m_forward;
	glm::vec3 m_up;
	glm::vec3 m_right;

	glm::uvec2 m_resolution;


	Camera(const glm::vec3& eye, const glm::vec3& focus, const glm::uvec2& resolution);

	void lookAt(const glm::vec3& focus, const glm::vec3& eye);
	void lookAt(const glm::vec3& focus);

	void move(const glm::vec3& offset);
	void pan(const glm::vec2& offset);
	void orbit(const glm::vec2& eulerOffset);
	void zoom(float scale);

	void update();
	glm::mat3 getAxes() const;

private:
	static constexpr float minYAngle = 10.f * M_PI / 180.f;
	static constexpr float maxYAngle = 170.f * M_PI / 180.f;
};
