#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#include "App.hpp"
#include "VkBootstrap.h"

class App {
public:
    bool init() {
        if (!glfwInit()) {
            std::cerr << "Could not initialize GLFW!\n";
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_glfwWindow = glfwCreateWindow(800, 600, "Atom3D", nullptr, nullptr);

        vkb::InstanceBuilder builder;
        auto inst_ret = builder
                            .set_app_name("Atom3D")
                            .request_validation_layers()
                            .use_default_debug_messenger()
                            .set_debug_messenger_severity(debug_severity)
                            .set_debug_messenger_type(debug_types)
                            .build();

        if (!inst_ret) {
            std::cerr << "Error creating instance: " << inst_ret.error().message() << "\n";
            return false;
        }

        m_vkbInstance = inst_ret.value();

        // Surface Creation
        VkSurfaceKHR tmp_surface = VK_NULL_HANDLE;
        VkResult glfw_ret = glfwCreateWindowSurface(m_vkbInstance, m_glfwWindow, nullptr, &tmp_surface);

        if (glfw_ret != VK_SUCCESS) {
            std::cerr << "Error creating window surface!\n";
            return false;
        }

        m_surface = tmp_surface;

        vkb::PhysicalDeviceSelector selector(m_vkbInstance);
        auto phys_ret = selector.set_surface(m_surface).select();

        if (!phys_ret) {
            std::cerr << "Failed to select physical device: " << phys_ret.error().message() << "\n";
            return false;
        }

        vkb::PhysicalDevice pd = phys_ret.value();

        vkb::DeviceBuilder device_builder{pd};
        auto device_ret = device_builder.build();

        if (!device_ret) {
            std::cerr << "Failed to create Logical Device: " << device_ret.error().message() << "\n";
            return false;
        }

        m_vkbDevice = device_ret.value();
        m_device = m_vkbDevice.device;

        if (!create_swapchain()) {
            return false;
        }

        if (!create_swapchain_image_views()) {
            return false;
        }

        return true;
    }

    bool create_swapchain() {
        vkb::SwapchainBuilder swapchain_builder{m_vkbDevice};

        auto swap_ret = swapchain_builder.set_old_swapchain(m_vkbSwapchain).build();
        if (!swap_ret) {
            std::cerr << "Error creating swapchain: " << swap_ret.error().message() << "\n";
            return false;
        }

        vkb::destroy_swapchain(m_vkbSwapchain);

        m_vkbSwapchain = swap_ret.value();

        return true;
    }

    bool create_swapchain_image_views() {
        auto tmpi = m_vkbSwapchain.get_images();

        if (!tmpi) {
            std::cerr << "Failed to acquire Images from swapchain! " << tmpi.error().message() << "\n";
            return false;
        }

        auto tmpiv = m_vkbSwapchain.get_image_views();

        if (!tmpiv) {
            std::cerr << "Failed to acquire ImageViews from swapchain! " << tmpiv.error().message() << "\n";
            return false;
        }

        return true;
    }

    bool recreate_swapchain() {
        m_vkbDevice.device;

        return true;
    }

    void windowLoop() {
        while (!glfwWindowShouldClose(m_glfwWindow)) {
            glfwPollEvents();
        }
    }

    void destroy_swapchain() {
        // Destroy Image Views
        m_vkbSwapchain.destroy_image_views(m_swapchain_image_views);

        // Destroy Framebuffers

        vkb::destroy_swapchain(m_vkbSwapchain);
    }

    void destroy() {
        destroy_swapchain();

        vkb::destroy_device(m_vkbDevice);
        vkb::destroy_surface(m_vkbInstance, m_surface);
        vkb::destroy_instance(m_vkbInstance);

        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
    }

private:
    // Vulkan Handles
    vk::Device m_device;
    vk::SurfaceKHR m_surface;
    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;

    // GLFW Handles
    GLFWwindow* m_glfwWindow;

    // VKB Handles
    vkb::Instance m_vkbInstance;
    vkb::Device m_vkbDevice;
    vkb::Swapchain m_vkbSwapchain;
    vkb::PhysicalDevice m_vkbPD;

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

int main() {
    App app;

    app.init();

    app.windowLoop();

    app.destroy();

    return 0;
}