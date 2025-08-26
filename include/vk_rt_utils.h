#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include <iostream>
import vulkan_hpp;

#define CHECK_VK_RESULT(res, msg) \
if(res != VK_SUCCESS) { \
	fprintf(stderr, "Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	exit(EXIT_FAILURE); \
}

const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);

const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);

#endif