#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"

#define CHECK_VK_RESULT(res, msg) \
if(res != VK_SUCCESS) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	throw std::runtime_error(msg); \
}

namespace PathTracingVK {

const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);

const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);

vk::raii::Semaphore CreateSemaphore(vk::raii::Device& device);

}
#endif