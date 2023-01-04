#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

using namespace GLCore;
using namespace GLCore::Utils;


class Ball
{
public:
	Ball() : weight(1.0f), radius(0.3f), 
		position(glm::vec3(0.0f, 0.0f, 0.0f)), 
		acceleration(glm::vec3(0.0f, 0.0f, 0.0f)),
		force(glm::vec3(0.0f, 0.0f, 0.0f)),
		velocity(glm::vec3(0.0f, 0.0f, 0.0f)),
		color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)) {}
	float weight;
	float radius;
	glm::vec3 position;
	glm::vec3 acceleration;
	glm::vec3 force;
	glm::vec3 velocity;
	glm::vec4 color;
private:
};
