#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "Vulkan.h"
#include "Vma.h"
#include "VulkanPhysicalDevices.h"
#include "VulkanQueue.h"
#include "../Pathtracer/Scene.h"

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
	VulkanQueue* GetQueue() { return m_queue.get(); }
	[[nodiscard]] uint32_t GetQueueFamily() const { return m_queueFamily; }
	[[nodiscard]] vk::Format GetSwapChainFormat() const { return m_swapChainSurfaceFormat.format; }
	[[nodiscard]] vk::Format GetDepthFormat() const { return m_physDevices.Selected().m_depthFormat; }

private:
	VmaAllocator m_allocator = nullptr;

	vk::raii::Context m_context;
	vk::raii::Instance m_instance = VK_NULL_HANDLE;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	vk::raii::SurfaceKHR m_surface = VK_NULL_HANDLE; // vulkan window abstraction
	vk::raii::Device m_device = VK_NULL_HANDLE;

	// Swapchain components
	vk::raii::SwapchainKHR m_swapChain = VK_NULL_HANDLE;
	vk::SurfaceFormatKHR m_swapChainSurfaceFormat{};
	std::vector<vk::Image> m_swapChainImages;
	std::vector<vk::raii::ImageView> m_swapChainImageViews;

	VulkanPhysicalDevices m_physDevices; // struct containing all available physical devices
	uint32_t m_queueFamily = 0; // selected queue family index
	std::unique_ptr<VulkanQueue> m_queue; // vulkan queue abstraction
	vk::raii::CommandPool m_cmdPool = VK_NULL_HANDLE;

	// Raytracing pipeline components
	vk::raii::Pipeline m_rtPipeline = VK_NULL_HANDLE;
	vk::raii::PipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
	std::vector<vk::raii::AccelerationStructureKHR> m_BLASs;
	vk::raii::AccelerationStructureKHR m_TLASs = VK_NULL_HANDLE;

	// Shader binding table stuff
	vk::raii::Buffer m_sbtBuffer = VK_NULL_HANDLE;
	std::vector<uint8_t> m_shaderHandles;
	vk::StridedDeviceAddressRegionKHR m_raygenRegion{};
	vk::StridedDeviceAddressRegionKHR m_missRegion{};
	vk::StridedDeviceAddressRegionKHR m_hitRegion{};
	vk::StridedDeviceAddressRegionKHR m_callableRegion{}; // callable shader region

	// Scene data
	const float m_vertices[9] = {
		0.25f, 0.25f, 0.0f,
		0.75f, 0.25f, 0.0f,
		0.50f, 0.75f, 0.0f
	};

	const uint32_t m_indices[3] = { 0, 1, 2 };

	struct {
		int Major = 0;
		int Minor = 0;
		int Patch = 0;
	} m_instanceVersion;

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* pWindow);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void InitVmaAllocator();
	void CreateSwapchain();
	void CreateCommandPool();

	void CreateBLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateTLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateSBT();
	void CreateAccelerationStructure(Scene scene);
	void CreateRaytracingPipeline();

	void UpdateInstanceVersion();
};

}

#endif