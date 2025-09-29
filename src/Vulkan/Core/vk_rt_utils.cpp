#include "vk_rt_utils.h"

namespace PathTracingVK {

const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		return "Verbose";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		return "Info";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		return "Warning";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		return "Error";

	default:
		throw std::runtime_error("Invalid severity code");
	}
}


const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type)
{
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
		return "General";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
		return "Validation";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
		return "Performance";
#ifdef _WIN64
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
		return "Device address binding";
#endif
	throw std::runtime_error("Invalid type code");
}

vk::raii::Semaphore CreateSemaphore(vk::raii::Device& device) {
	return vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
}

}