#ifndef VULKAN_H
#define VULKAN_H

import vulkan_hpp;
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <stdexcept>
#include <optional>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
constexpr bool enableDebugging = false;
#else
constexpr bool enableDebugging = true;
#endif

struct InstanceVersion {
    int Major = 0;
    int Minor = 0;
    int Patch = 0;
};

#endif