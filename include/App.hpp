#ifndef APP_HPP
#define APP_HPP

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// VULKAN
#include <VkBootstrap.h>

#include <vulkan/vulkan.hpp>

// STD
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <deque>

#include "Vertex.hpp"
#include "AllocatedBuffer.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

struct DeletionQueue {
    std::deque<vk::Buffer> buffers;
    std::deque<vk::Image> images;
    std::deque<vk::Image> imageViews;

    void flush() {
        auto bit = buffers.rbegin();
        auto iit = images.rbegin();
        auto ivit = imageViews.rbegin();

        while (buffers.size() != 0 && images.size() != 0 && imageViews.size() != 0) {
            if (bit != buffers.rend()) bit++;
            if (iit != images.rend()) iit++;
            if (ivit != imageViews.rend()) ivit++;
        }
    }
};

class App {
public:
    bool init();
    
    bool createSwapchain();
    bool createSwapchainImageViews();
    bool recreateSwapchain();
    void cleanupSwapchain();
    
    bool getQueues();
    
    void createRenderPass();
    
    std::vector<char> loadSPV(const std::string& filename);
    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    bool createGraphicsPipeline();
    
    bool createFramebuffers();
    
    void createCommandPool();
    void createCommandBuffers();
    
    void createSyncObjects();
    void destroySyncObjects();
    
    bool drawFrame();
    void windowLoop();

    void destroy();

    AllocatedBuffer createBuffer(size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memUsage);

public:
    bool glfwFramebufferResized;

private:
    // Vulkan Handles
    vk::Instance m_instance;
    vk::Device m_device;
    vk::PhysicalDevice m_physDevice;
    vk::SurfaceKHR m_surface;
    vk::Queue m_graphicsQueue, m_presentQueue;
    vk::RenderPass m_renderPass;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
    vk::CommandPool m_commandPool;
    vk::SwapchainKHR m_swapchain;
    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<vk::Framebuffer> m_framebuffers;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    // Sync
    std::vector<vk::Semaphore> m_imageAvailableSems;
    std::vector<vk::Semaphore> m_renderFinishedSems;
    std::vector<vk::Fence> m_inFlightFences;
    std::vector<vk::Fence> m_imageInFlightFences;

    // GLFW Handles
    GLFWwindow* m_glfwWindow;

    // VKB Handles
    vkb::Instance m_vkbInstance;
    vkb::Device m_vkbDevice;
    vkb::Swapchain m_vkbSwapchain;
    vkb::PhysicalDevice m_vkbPD;

    // VMA Handles
    vma::Allocator m_vmaAllocator;

    // Atom Shtuff
    DeletionQueue m_delQueue;

    int m_currentFrame = 0;

    // Other Vulkan Things
    const VkDebugUtilsMessageSeverityFlagsEXT debug_severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    const VkDebugUtilsMessageTypeFlagsEXT debug_types =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
};

#endif