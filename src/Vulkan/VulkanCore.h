#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "Vulkan.h"
#include "vk_mem_alloc.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "../Pathtracer/Scene.h"
#include "VulkanWindow.h"
#include "VulkanPhysicalDevice.h"
#include "vulkan/vulkan_raii.hpp"

namespace PathTracingVk {

class VulkanCore {
public:
	VulkanCore(const char *appName, const VulkanWindow& window);
	~VulkanCore();

	void DeviceWaitIdle();
	void RecreateSwapchain();

	vk::raii::CommandBuffer& BeginCommandBuffer();
	void SubmitAsync(const vk::CommandBuffer &cmdBuff);

	void Present();

	[[nodiscard]] vk::Format GetDepthFormat() const { return m_physDevice->m_depthFormat; }
	[[nodiscard]] vk::raii::Queue* GetQueue() { return &m_queue; }
	[[nodiscard]] const VulkanSwapchain* GetSwapchain() const { return m_swapchain.get(); }
	[[nodiscard]] const VulkanDevice* GetDevice() const { return m_device.get(); }

private:


	const VulkanWindow& m_window;
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = VK_NULL_HANDLE;
	vk::raii::SurfaceKHR m_surface = VK_NULL_HANDLE; // vulkan window abstraction
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	std::unique_ptr<VulkanPhysicalDevice> m_physDevice;
	std::unique_ptr<VulkanDevice> m_device;
	vk::raii::Queue m_queue = VK_NULL_HANDLE; // graphics queue
	std::unique_ptr<VulkanSwapchain> m_swapchain;

	VmaAllocator m_allocator = nullptr;

	// currently there are only pools and buffers for graphics queue
	std::vector<vk::raii::CommandPool> m_cmdPools;
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs;

	// frame rendering objects
	std::vector<vk::raii::Semaphore> m_presentSemaphores;
	std::vector<vk::raii::Semaphore> m_renderSemaphores;
	std::vector<vk::raii::Fence> m_inFlightFences;
	uint32_t m_currentFrameIndex = 0;
	uint32_t m_currentImageIndex = 0;

	// Raytracing pipeline components
	vk::raii::Pipeline m_rtPipeline = VK_NULL_HANDLE;
	vk::raii::PipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
	std::vector<vk::raii::AccelerationStructureKHR> m_blas;
	vk::raii::AccelerationStructureKHR m_tlas = VK_NULL_HANDLE;

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

	void CreateInstance(const char* appName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* window);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void InitVmaAllocator();
	void CreateSwapchain();
	void CreateSyncObjects();
	void CreateCommandObjects();

	vk::raii::CommandBuffer &PrepareFrame();

	void SubmitFrame();

	void CreateBLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateTLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateSBT();
	void CreateAccelerationStructure(Scene scene);
	void CreateRaytracingPipeline();

	void UpdateInstanceVersion();
};

}

#endif