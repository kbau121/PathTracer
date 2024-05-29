#include <camera.h>

#include <cmath>
#include <glm/gtx/euler_angles.hpp>

// Creates a new unfinalized camera object
Camera::Camera(const glm::vec3& eye, const glm::vec3& focus, const glm::uvec2& resolution)
	: m_eye(eye), m_focus(focus), m_resolution(resolution)
{ }

// Sets the camera focus and position
void Camera::lookAt(const glm::vec3& focus, const glm::vec3& eye)
{
	m_focus = focus;
	m_eye = eye;
}

// Sets the camera focus
void Camera::lookAt(const glm::vec3& focus)
{
	m_focus = focus;
}

// Moves the camera and focus
void Camera::move(const glm::vec3& offset)
{
	m_eye += offset;
	m_focus += offset;
}

// Moves the camera and focus along the camera plane [horizontal, vertical]
void Camera::pan(const glm::vec2& offset)
{
	move(m_right * offset.x + m_up * offset.y);
}

// Rotates the camera about the focus by radian angles [horizontal, vertical]
void Camera::orbit(const glm::vec2& eulerAngles)
{
	glm::vec3 offset = m_eye - m_focus;
	float distance = glm::length(offset);
	glm::vec3 outDir = offset / distance;

	// x = cos(G)*sin(T)
	// y = cos(T)
	// z = sin(G)*sin(T)

	float theta = glm::acos(outDir.y);
	float gamma;

	float sinT = glm::sin(theta);
	if (sinT != 0.f)
	{
		gamma = glm::acos(outDir.x / glm::sin(theta));
	}
	else
	{
		gamma = glm::atan(outDir.z / outDir.x);
	}

	if (outDir.z < 0.f)
	{
		gamma = M_PI * 2.f - gamma;
	}
	
	theta += eulerAngles.y;
	gamma += eulerAngles.x;

	theta = max(minYAngle, min(theta, maxYAngle));
	
	outDir.y = glm::cos(theta);
	outDir.x = glm::cos(gamma) * glm::sin(theta);
	outDir.z = glm::sin(gamma) * glm::sin(theta);

	m_eye = m_focus + outDir * distance;
}

// Moves the camera further from the focus by a factor of [scale]
void Camera::zoom(float scale)
{
	glm::vec3 offset = m_eye - m_focus;
	float distance = glm::length(offset);
	glm::vec3 outDir = offset / distance;

	m_eye = m_focus + outDir * distance * scale;
}

// Updates the stored axes
void Camera::update()
{
	m_forward = glm::normalize(m_focus - m_eye);
	// Crossed with <0, 1, 0>
	m_right = glm::normalize(glm::vec3(-m_forward.z, 0.f, m_forward.x));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
}

// Gets the 3x3 matrix representing the camera axes [x|y|z]
glm::mat3 Camera::getAxes() const
{
	return glm::mat3(m_right, m_up, m_forward);
}
