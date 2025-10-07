#ifndef VULKAN_H
#define VULKAN_H

import vulkan_hpp;
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <stdexcept>
#include <optional>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#endif