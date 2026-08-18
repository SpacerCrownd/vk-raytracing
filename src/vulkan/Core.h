#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "Vulkan.h"
#include "ResourceAllocator.h"
#include "Device.h"
#include "Swapchain.h"
#include "Window.h"
#include "PhysicalDevice.h"
#include "Shader.h"

namespace ptvk {

class Core {
public:
	Core(const char *appName, const Window& window);
	~Core() = default;

	bool framebufferResized = false;

	void deviceWaitIdle();
	void recreateSwapchain();

	vk::Format               getDepthFormat() const { return m_pPhysDevice->m_depthFormat; }
	vk::raii::Queue&         getQueue() { return m_queue; }
	const Swapchain&         getSwapchain() const { return *m_pSwapchain; }
	const Device&            getDevice() const { return *m_pDevice; }
	uint32_t                 getCurrentFrameIndex() const { return m_currentFrameIndex; }
	uint32_t                 getCurrentImageIndex() const { return m_currentImageIndex; }
	const ResourceAllocator& getResourceAllocator() const { return *m_pResourceAllocator; }
	const Image&             getDrawImage() const { return m_drawImage; }
	const Image&             getDepthImage(uint32_t i) const { return m_depthImages[i]; }

	vk::raii::CommandBuffer& beginCommandRecording();

	void prepareFrame();
	void submitFrame();
	void presentFrame();

private:
	InstanceVersion m_instanceVersion;

	const Window&                    m_window;
	vk::raii::Context                m_context{};
	vk::raii::Instance               m_instance{VK_NULL_HANDLE};
	vk::raii::SurfaceKHR             m_surface{VK_NULL_HANDLE}; // vulkan window abstraction
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};

	std::unique_ptr<PhysicalDevice>    m_pPhysDevice{};
	std::unique_ptr<Device>            m_pDevice{};
	std::unique_ptr<Swapchain>         m_pSwapchain{};
	std::unique_ptr<ResourceAllocator> m_pResourceAllocator{};

	Image              m_drawImage;
	std::vector<Image> m_depthImages;

	// graphics queue
	vk::raii::Queue m_queue{VK_NULL_HANDLE};

	// per frame in flight command objects
	std::vector<vk::raii::CommandPool>   m_cmdPools{};
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs{};

	vk::raii::CommandPool m_transientCmdPool{VK_NULL_HANDLE};

	// per frame in flight rendering objects
	std::vector<vk::raii::Semaphore> m_presentSemaphores{};
	std::vector<vk::raii::Semaphore> m_renderSemaphores{};
	std::vector<vk::raii::Fence>     m_inFlightFences{};

	uint32_t m_currentFrameIndex{0};
	uint32_t m_currentImageIndex{0};

	// -- Raytracing objects --
	// Raytracing pipeline components
	vk::raii::Pipeline                              m_rtPipeline{VK_NULL_HANDLE};
	vk::raii::PipelineLayout                        m_rtPipelineLayout{VK_NULL_HANDLE};
	std::vector<vk::raii::AccelerationStructureKHR> m_blas{};
	vk::raii::AccelerationStructureKHR              m_tlas{VK_NULL_HANDLE};

	// Shader binding table stuff
	vk::raii::Buffer                  m_sbtBuffer{VK_NULL_HANDLE};
	std::vector<uint8_t>              m_shaderHandles{};
	vk::StridedDeviceAddressRegionKHR m_raygenRegion{};
	vk::StridedDeviceAddressRegionKHR m_missRegion{};
	vk::StridedDeviceAddressRegionKHR m_hitRegion{};
	vk::StridedDeviceAddressRegionKHR m_callableRegion{}; // callable shader region

	void createInstance(const char* appName);
	void createDebugCallback();
	void createSurface(GLFWwindow* window);
	void selectPhysicalDevice();
	void createLogicalDevice();
	void initResourceAllocator();
	void createSwapchain();
	void createSyncObjects();
	void createCommandObjects();
	void createDepthResources();

	// Raytracing initialization methods
	void createBLAS(vk::raii::CommandBuffer &cmdBuff);
	void createTLAS(vk::raii::CommandBuffer &cmdBuff);
	void createSBT();
	void createAccelerationStructure();
	void createRaytracingPipeline();

	void		 updateInstanceVersion();
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
};
}

#endif