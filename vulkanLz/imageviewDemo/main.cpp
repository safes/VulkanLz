#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include "Sample01Application.h"

int main()
{
	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	Sample01Application app;
	app.run();

	
	return 0;

}