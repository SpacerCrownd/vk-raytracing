#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "../Window.h"

#include "Vulkan.h"
#include "ResourceAllocator.h"
#include "Device.h"
#include "Swapchain.h"
#include "PhysicalDevice.h"

namespace ptvk {
class Core {
public:
	Core(const char *appName, const app::Window& window);
	~Core() = default;

	bool resizeRequested = false;

	void deviceWaitIdle();
	void recreateSwapchain();

	vk::Format					getDepthFormat() const { return m_pPhysDevice->m_depthFormat; }
	vk::raii::Queue&			getQueue() { return m_queue; }
	const Swapchain&			getSwapchain() const { return *m_pSwapchain; }
	const Device&				getDevice() const { return *m_pDevice; }
	GLFWwindow*					getWindow() const { return m_window.getWindow(); }
	uint32_t					getCurrentFrameIndex() const { return m_currentFrameIndex; }
	uint32_t					getCurrentImageIndex() const { return m_currentImageIndex; }
	const ResourceAllocator&	getResourceAllocator() const { return *m_pResourceAllocator; }
	const Image&				getDrawImage() const { return m_drawImage; }
	const Image&				getAccumulationImage() const { return m_accImage; }
	const Image&				getAlbedoImage() const { return m_albedoImage; }
	const Image&				getNormalImage() const { return m_normalImage; }
	const Image&				getDepthImage() const { return m_depthImage; }
	const vk::Instance&			getInstance() const { return *m_instance; }
	uint32_t getInstanceVersion() const { return vk::makeApiVersion(0, m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch); }

	void getFramebufferSize(int &width, int &height) const { return glfwGetWindowSize(m_window.getWindow(), &width, &height);}

	vk::raii::CommandBuffer& beginCommandRecording();

	vk::raii::CommandBuffer beginSingleTimeCommandBuffer();
	vk::Result				submitSingleTimeCommandBuffer(const vk::raii::CommandBuffer& cmdBuf);

	bool prepareFrame(); // return true if successfully acquired new swapchain image, false if resized and recreated swapchain
	void submitFrame();
	void presentFrame();

private:
	InstanceVersion m_instanceVersion;

	const app::Window&               m_window;
	vk::raii::Context                m_context{};
	vk::raii::Instance               m_instance{VK_NULL_HANDLE};
	vk::raii::SurfaceKHR             m_surface{VK_NULL_HANDLE}; // vulk window abstraction
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};

	std::unique_ptr<PhysicalDevice>    m_pPhysDevice{};
	std::unique_ptr<Device>            m_pDevice{};
	std::unique_ptr<ResourceAllocator> m_pResourceAllocator{};
	std::unique_ptr<Swapchain>         m_pSwapchain{};

	Image m_drawImage{};
	Image m_accImage{};
	Image m_albedoImage{};
	Image m_normalImage{};
	Image m_depthImage{};

	// graphics queue
	vk::raii::Queue m_queue{VK_NULL_HANDLE};

	// per frame in flight command objects
	std::vector<vk::raii::CommandPool>   m_cmdPools{};
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs{};

	vk::raii::CommandPool m_transientCmdPool{VK_NULL_HANDLE};

	// per frame in flight resources
	std::vector<vk::raii::Semaphore> m_presentSemaphores{};
	std::vector<vk::raii::Semaphore> m_renderSemaphores{};
	std::vector<vk::raii::Fence>     m_inFlightFences{};

	uint32_t m_currentFrameIndex{0};
	uint32_t m_currentImageIndex{0};

	void createInstance(const char* appName);
	void createDebugCallback();
	void createSurface(GLFWwindow* window);
	void selectPhysicalDevice();
	void createLogicalDevice();
	void initResourceAllocator();
	void createSwapchain();
	void createSyncObjects();
	void createCommandObjects();

	// helper functions
	void		 updateInstanceVersion();
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
};
}

#endif