#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "Vulkan.h"
#include "ResourceAllocator.h"
#include "Device.h"
#include "Swapchain.h"
#include "Window.h"
#include "PhysicalDevice.h"
#include "Shader.h"
#include "GraphicsPipeline.h"

namespace ptvk {

class Core {
public:
	Core(const char *appName, const Window& window);
	~Core() = default;

	void DeviceWaitIdle();
	void RecreateSwapchain();

	[[nodiscard]] vk::Format GetDepthFormat() const { return m_physDevice->m_depthFormat; }
	[[nodiscard]] vk::raii::Queue& GetQueue() { return m_queue; }
	[[nodiscard]] const Swapchain& GetSwapchain() const { return *m_swapchain; }
	[[nodiscard]] const Device& GetDevice() const { return *m_device; }
	[[nodiscard]] uint32_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }
	[[nodiscard]] uint32_t GetCurrentImageIndex() const { return m_currentImageIndex; }
	[[nodiscard]] const ResourceAllocator& GetResourceAllocator() const { return *m_resourceAllocator; }

	vk::raii::CommandBuffer& BeginCommandRecording();

	void PrepareFrame();
	void SubmitFrame();
	void PresentFrame();

	void CreateGraphicsPipeline(Shader rasterShader);

private:
	const Window& m_window;
	vk::raii::Context m_context{};
	vk::raii::Instance m_instance{VK_NULL_HANDLE};
	vk::raii::SurfaceKHR m_surface{VK_NULL_HANDLE}; // vulkan window abstraction
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};

	std::unique_ptr<PhysicalDevice> m_physDevice{};
	std::unique_ptr<Device> m_device{};
	std::unique_ptr<Swapchain> m_swapchain{};

	vk::raii::Queue m_queue{VK_NULL_HANDLE}; // graphics queue

	std::vector<vk::raii::CommandPool> m_cmdPools{};
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs{};

	// frame rendering objects
	std::vector<vk::raii::Semaphore> m_presentSemaphores{};
	std::vector<vk::raii::Semaphore> m_renderSemaphores{};
	std::vector<vk::raii::Fence> m_inFlightFences{};
	uint32_t m_currentFrameIndex{0};
	uint32_t m_currentImageIndex{0};

	// Rasterization
	std::unique_ptr<GraphicsPipeline> m_graphicsPipeline;

	// Raytracing pipeline components
	vk::raii::Pipeline m_rtPipeline{VK_NULL_HANDLE};
	vk::raii::PipelineLayout m_rtPipelineLayout{VK_NULL_HANDLE};
	std::vector<vk::raii::AccelerationStructureKHR> m_blas{};
	vk::raii::AccelerationStructureKHR m_tlas{VK_NULL_HANDLE};

	// Shader binding table stuff
	vk::raii::Buffer m_sbtBuffer{VK_NULL_HANDLE};
	std::vector<uint8_t> m_shaderHandles{};
	vk::StridedDeviceAddressRegionKHR m_raygenRegion{};
	vk::StridedDeviceAddressRegionKHR m_missRegion{};
	vk::StridedDeviceAddressRegionKHR m_hitRegion{};
	vk::StridedDeviceAddressRegionKHR m_callableRegion{}; // callable shader region

	vk::raii::CommandPool m_transientCmdPool{VK_NULL_HANDLE};

	std::unique_ptr<ResourceAllocator> m_resourceAllocator{};

	InstanceVersion m_instanceVersion;

	void CreateInstance(const char* appName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* window);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void InitResourceAllocator();
	void CreateSwapchain();
	void CreateSyncObjects();
	void CreateCommandObjects();

	vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	void CreateBLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateTLAS(vk::raii::CommandBuffer &cmdBuff);
	void CreateSBT();
	void CreateAccelerationStructure();
	void CreateRaytracingPipeline();

	void UpdateInstanceVersion();
};

}

#endif