#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "Vulkan.h"
#include "vk_mem_alloc.h"
#include "VulkanPhysicalDevices.h"
#include "VulkanQueue.h"

namespace PathTracingVK {

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class VulkanCore {

public:
	VulkanCore();
	~VulkanCore();

	void Init(const char* pAppName, GLFWwindow* pWindow);
	void CreateCommandBuffers(uint32_t count, std::vector<vk::raii::CommandBuffer>& cmdBuffs);
	static void FreeCommandBuffers(std::vector<vk::raii::CommandBuffer> &cmdBuffs);
	void Destroy();

	int GetNumImages() { return static_cast<int>(m_swapChainImages.size()); }
	vk::Image GetImage(int n) { return m_swapChainImages[n]; };
	VulkanQueue* GetQueue() { return std::addressof(m_queue.value()); }
	uint32_t GetQueueFamily() const { return m_queueFamily; }
	vk::Format GetSwapChainFormat() const { return m_swapChainSurfaceFormat.format; }
	vk::Format GetDepthFormat() const { return m_physDevices.Selected().m_depthFormat; }

private:
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = VK_NULL_HANDLE;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	// vulkan window abstraction
	vk::raii::SurfaceKHR m_surface = VK_NULL_HANDLE;
	vk::raii::Device m_device = VK_NULL_HANDLE;
	vk::raii::SwapchainKHR m_swapChain = VK_NULL_HANDLE;
	vk::SurfaceFormatKHR m_swapChainSurfaceFormat{};
	std::vector<vk::Image> m_swapChainImages;
	std::vector<vk::raii::ImageView> m_swapChainImageViews;
	vk::raii::CommandPool m_cmdPool = VK_NULL_HANDLE;

	// struct containing all available physical devices
	VulkanPhysicalDevices m_physDevices;
	// selected queue family index
	uint32_t m_queueFamily = 0;
	// vulkan queue abstraction
	std::optional<VulkanQueue> m_queue;

	struct {
		int Major = 0;
		int Minor = 0;
		int Patch = 0;
	} m_instanceVersion;

	VmaAllocator m_allocator = nullptr;

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* pWindow);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void InitVmaAllocator();
	void CreateSwapChain();
	void CreateCommandPool();

	void CreateBLAS();
	void CreateTLAS();
	void CreateSBT();
	void CreateAccelerationStructure();
	void CreateRaytracingPipeline();

	void UpdateInstanceVersion();
};

}

#endif