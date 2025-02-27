#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(WIN32) 
#include <Windows.h>

const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
#else if defined(__AAPL__)
#endif

#include "App.hpp"
#include "VkBootstrap.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

// Custom Debug Callback
VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                       VkDebugUtilsMessageTypeFlagsEXT type,
                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                       void* pUserData) {
    auto sevStr = vkb::to_string_message_severity(severity);
    auto typeStr = vkb::to_string_message_type(type);

#if defined(WIN32)
    int color;

    switch (severity) { 
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        color = 36;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        color = 32;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        color = 36;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        color = 31;
        break;
    default:
        color = 37;
        break;
    }

    std::cout << "\x1b[" << color << "m[" << sevStr << ": " << typeStr << "]: ";

    std::cout << "\x1b[37m" << pCallbackData->pMessage << "\n";
#else if defined(__AAPL__)
    printf("[%s : %s] : %s\n", sevStr, typeStr, pCallbackData->pMessage);
#endif

    return VK_FALSE;
}

class App {
public:
    bool init() {
        if (!glfwInit()) {
            std::cerr << "Could not initialize GLFW!\n";
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        m_glfwWindow = glfwCreateWindow(800, 600, "Atom3D", nullptr, nullptr);

        vkb::InstanceBuilder builder;
        auto instRet = builder
                           .set_app_name("Atom3D")
                           .request_validation_layers()
                            .set_debug_callback(debugCallback)
                           .set_debug_messenger_severity(debug_severity)
                           .set_debug_messenger_type(debug_types)
                           .build();

        if (!instRet) {
            std::cerr << "Error creating instance: " << instRet.error().message() << "\n";
            return false;
        }

        m_vkbInstance = instRet.value();
        m_instance = m_vkbInstance;

        // Surface Creation
        VkSurfaceKHR tmpSurface = VK_NULL_HANDLE;
        VkResult glfwRet = glfwCreateWindowSurface(m_vkbInstance, m_glfwWindow, nullptr, &tmpSurface);

        if (glfwRet != VK_SUCCESS) {
            std::cerr << "Error creating window surface!\n";
            return false;
        }

        m_surface = tmpSurface;

        vkb::PhysicalDeviceSelector selector(m_vkbInstance);
        auto physRet = selector.set_surface(m_surface).select();

        if (!physRet) {
            std::cerr << "Failed to select physical device: " << physRet.error().message() << "\n";
            return false;
        }

        vkb::PhysicalDevice pd = physRet.value();

        vkb::DeviceBuilder device_builder{pd};
        auto deviceRet = device_builder.build();

        if (!deviceRet) {
            std::cerr << "Failed to create Logical Device: " << deviceRet.error().message() << "\n";
            return false;
        }

        m_vkbDevice = deviceRet.value();
        m_device = m_vkbDevice.device;

        if (!createSwapchain()) {
            return false;
        }

        if (!createSwapchainImageViews()) {
            return false;
        }

        if (!getQueues()) {
            return false;
        }

        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();

        return true;
    }

    bool createSwapchain() {
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

    bool createSwapchainImageViews() {
        auto tmpi = m_vkbSwapchain.get_images();

        if (!tmpi) {
            std::cerr << "Failed to acquire Images from swapchain! " << tmpi.error().message() << "\n";
            return false;
        }

        m_swapchainImages = tmpi.value();

        auto tmpiv = m_vkbSwapchain.get_image_views();

        if (!tmpiv) {
            std::cerr << "Failed to acquire ImageViews from swapchain! " << tmpiv.error().message() << "\n";
            return false;
        }

        m_swapchainImageViews = tmpiv.value();

        return true;
    }

    bool recreateSwapchain() {
        destroySwapchainDependants();

        createSwapchain();
        createSwapchainImageViews();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();

        return true;
    }

    void destroySwapchainDependants(const bool bingbong = false) {
        m_device.waitIdle();

        m_device.freeCommandBuffers(m_commandPool, m_commandBuffers);
        m_device.destroyCommandPool(m_commandPool);

        // Destroy Framebuffers
        for (auto& f : m_framebuffers) {
            m_device.destroyFramebuffer(f);
        }

        // Destroy Image Views
        m_vkbSwapchain.destroy_image_views(m_swapchainImageViews);

        if (bingbong) vkb::destroy_swapchain(m_vkbSwapchain);
    }

    bool getQueues() {
        auto gq = m_vkbDevice.get_queue(vkb::QueueType::graphics);
        if (!gq) {
            std::cerr << "Failed to acquire graphics queue: " << gq.error().message() << "\n";
            return false;
        }

        m_graphicsQueue = gq.value();

        auto pq = m_vkbDevice.get_queue(vkb::QueueType::present);
        if (!pq) {
            std::cerr << "Failed to acquire present queue: " << pq.error().message() << "\n";
            return false;
        }

        m_presentQueue = pq.value();

        return true;
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment;
        colorAttachment.setFormat(static_cast<vk::Format>(m_vkbSwapchain.image_format))
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);

        vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorRef);

        vk::SubpassDependency dep(vk::SubpassExternal, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::AccessFlagBits::eNone,
                                  vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        vk::RenderPassCreateInfo renderPassInfo(vk::RenderPassCreateFlags(),
                                                1, &colorAttachment,
                                                1, &subpass,
                                                1, &dep);

        m_renderPass = m_device.createRenderPass(renderPassInfo);
    }

    std::vector<char> loadSPV(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Failed to open shader " << fileName << "\n";
            throw std::runtime_error("oop");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buf(fileSize);

        file.seekg(0);
        file.read(buf.data(), fileSize);

        file.close();

        return buf;
    }

    vk::ShaderModule createShaderModule(const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo info(vk::ShaderModuleCreateFlagBits(),
                                        code.size(),
                                        reinterpret_cast<const uint32_t*>(code.data()));

        return m_device.createShaderModule(info);
    }

    bool createGraphicsPipeline() {
        // Shaders
        auto vertCode = loadSPV("../src/shaders/vert.spv");
        auto fragCode = loadSPV("../src/shaders/frag.spv");

        auto vertModule = createShaderModule(vertCode);
        auto fragModule = createShaderModule(fragCode);

        if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
            std::cerr << "Failed to create shader module";
            return false;
        }

        vk::PipelineShaderStageCreateInfo vertInfo({}, vk::ShaderStageFlagBits::eVertex, vertModule, "main");
        vk::PipelineShaderStageCreateInfo fragInfo({}, vk::ShaderStageFlagBits::eFragment, fragModule, "main");

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {vertInfo, fragInfo};

        // Input States
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, vk::PrimitiveTopology::eTriangleList, false);

        // viewport stuff
        vk::Viewport viewport(0, 0,
                              (float)m_vkbSwapchain.extent.width,
                              (float)m_vkbSwapchain.extent.height,
                              0.0, 1.0);

        vk::Rect2D scissor({0, 0}, m_vkbSwapchain.extent);

        vk::PipelineViewportStateCreateInfo viewportInfo({}, viewport, scissor);

        // Rasterizer
        vk::PipelineRasterizationStateCreateInfo rasterizer({}, false, false,
                                                            vk::PolygonMode::eFill,
                                                            vk::CullModeFlagBits::eBack,
                                                            vk::FrontFace::eClockwise,
                                                            false, 0.0, 0.0, 0.0, 1.0);

        // MultiSampling
        vk::PipelineMultisampleStateCreateInfo multisampling;

        // Color Blending
        vk::PipelineColorBlendAttachmentState colorblendAttachment;
        colorblendAttachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;
        colorblendAttachment.blendEnable = false;

        vk::PipelineColorBlendStateCreateInfo colorblendInfo;
        colorblendInfo.setLogicOpEnable(false)
            .setLogicOp(vk::LogicOp::eCopy)
            .setAttachmentCount(1)
            .setPAttachments(&colorblendAttachment);

        // Layout
        vk::PipelineLayoutCreateInfo layoutInfo;

        m_pipelineLayout = m_device.createPipelineLayout(layoutInfo);

        std::vector<vk::DynamicState> dynStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        vk::PipelineDynamicStateCreateInfo dynamicInfo({}, dynStates);

        // Pipeline
        vk::GraphicsPipelineCreateInfo pipeInfo;
        pipeInfo.setStageCount(2)
            .setStages(shaderStages)
            .setPVertexInputState(&vertexInputInfo)
            .setPInputAssemblyState(&inputAssemblyInfo)
            .setPViewportState(&viewportInfo)
            .setRenderPass(m_renderPass)
            .setPRasterizationState(&rasterizer)
            .setPMultisampleState(&multisampling)
            .setPColorBlendState(&colorblendInfo)
            .setPDynamicState(&dynamicInfo)
            .setLayout(m_pipelineLayout)
            .setSubpass(0);

        auto tmp = m_device.createGraphicsPipeline(VK_NULL_HANDLE, pipeInfo);

        if (tmp.result != vk::Result::eSuccess) {
            std::cerr << "Failed to create graphics pipeline.";
            return false;
        }

        m_graphicsPipeline = tmp.value;

        m_device.destroyShaderModule(vertModule);
        m_device.destroyShaderModule(fragModule);

        return true;
    }

    bool createFramebuffers() {
        m_framebuffers.resize(0);
        m_framebuffers.reserve(m_swapchainImageViews.size());

        for (int i = 0; i < m_swapchainImageViews.size(); i++) {
            vk::ImageView aaa = m_swapchainImageViews[i];

            vk::FramebufferCreateInfo info;
            info.setRenderPass(m_renderPass)
                .setAttachmentCount(1)
                .setAttachments(aaa)
                .setWidth(m_vkbSwapchain.extent.width)
                .setHeight(m_vkbSwapchain.extent.height)
                .setLayers(1);

            m_framebuffers.push_back(m_device.createFramebuffer(info));
        }

        return true;
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo info({}, m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value());

        m_commandPool = m_device.createCommandPool(info);
    }

    // Records too
    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, m_framebuffers.size());

        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);

        vk::CommandBufferBeginInfo beginInfo;
        vk::ClearValue clearVal;
        clearVal.setColor(vk::ClearColorValue(0, 0, 0, 1));

        for (size_t i = 0; i < m_commandBuffers.size(); i++) {
            m_commandBuffers[i].begin(beginInfo);

            vk::RenderPassBeginInfo rpInfo(m_renderPass, m_framebuffers[i], {{0, 0}, {m_vkbSwapchain.extent}}, clearVal);

            vk::Viewport viewport((0.0), (0.0), m_vkbSwapchain.extent.width, m_vkbSwapchain.extent.height, 0, 1);
            vk::Rect2D scissor({0, 0}, m_vkbSwapchain.extent);

            m_commandBuffers[i].setViewport(0, viewport);
            m_commandBuffers[i].setScissor(0, scissor);

            m_commandBuffers[i].beginRenderPass(rpInfo, vk::SubpassContents::eInline);

            m_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

            m_commandBuffers[i].draw(3, 1, 0, 0);

            m_commandBuffers[i].endRenderPass();

            m_commandBuffers[i].end();
        }
    }

    void createSyncObjects() {
        m_imageAvailableSems.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSems.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_imageInFlightFences.resize(m_vkbSwapchain.image_count, VK_NULL_HANDLE);

        vk::SemaphoreCreateInfo semInfo;

        vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSems[i] = m_device.createSemaphore(semInfo);
            m_renderFinishedSems[i] = m_device.createSemaphore(semInfo);
            m_inFlightFences[i] = m_device.createFence(fenceInfo);
        }
    }

    bool drawFrame() {
        auto fenceWait = m_device.waitForFences(m_inFlightFences[m_currentFrame], true, UINT64_MAX);

        auto imageIndex = m_device.acquireNextImageKHR(m_vkbSwapchain.swapchain,
                                                       UINT64_MAX,
                                                       m_imageAvailableSems[m_currentFrame],
                                                       VK_NULL_HANDLE);

        if (imageIndex.result == vk::Result::eErrorOutOfDateKHR) {
            return recreateSwapchain();
        } else if (imageIndex.result != vk::Result::eSuccess && imageIndex.result != vk::Result::eSuboptimalKHR) {
            std::cerr << "Failed to acquire swapchain image. Error: " << imageIndex.result << "\n";
            return false;
        }

        if (m_imageInFlightFences[imageIndex.value] != VK_NULL_HANDLE) {
            fenceWait = m_device.waitForFences(m_imageInFlightFences[imageIndex.value], true, UINT64_MAX);
        }

        m_imageInFlightFences[imageIndex.value] = m_inFlightFences[m_currentFrame];

        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

        vk::SubmitInfo subInfo;
        subInfo.setWaitSemaphores(m_imageAvailableSems[m_currentFrame])
            .setWaitDstStageMask(waitStages)
            .setCommandBuffers(m_commandBuffers[m_currentFrame])
            .setSignalSemaphores(m_renderFinishedSems[m_currentFrame]);

        m_device.resetFences(m_inFlightFences[m_currentFrame]);

        m_graphicsQueue.submit(subInfo, m_inFlightFences[m_currentFrame]);

        vk::SwapchainKHR tmpswpch = m_vkbSwapchain.swapchain;

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphores(m_renderFinishedSems[m_currentFrame])
            .setSwapchains(tmpswpch)
            .setImageIndices(imageIndex.value);

        auto r = m_presentQueue.presentKHR(presentInfo);

        if (r == vk::Result::eErrorOutOfDateKHR ||
            r == vk::Result::eSuboptimalKHR) {
            return recreateSwapchain();
        } else if (r != vk::Result::eSuccess) {
            std::cerr << "Failed to present swapchain image.\n";
            return false;
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return true;
    }

    void windowLoop() {
        while (!glfwWindowShouldClose(m_glfwWindow)) {
            glfwPollEvents();

            if (!drawFrame()) {
                std::cerr << "Failed to draw frame\n";
                return;
            }
        }
    }

    void destroy() {
        m_device.waitIdle();

        for (auto& s : m_imageAvailableSems)
            m_device.destroySemaphore(s);

        for (auto& s : m_renderFinishedSems)
            m_device.destroySemaphore(s);

        for (auto& f : m_inFlightFences)
            m_device.destroyFence(f);

        destroySwapchainDependants(true);

        m_device.destroyPipeline(m_graphicsPipeline);
        m_device.destroyPipelineLayout(m_pipelineLayout);
        m_device.destroyRenderPass(m_renderPass);

        m_device = VK_NULL_HANDLE;
        m_instance = VK_NULL_HANDLE;

        vkb::destroy_device(m_vkbDevice);
        vkb::destroy_surface(m_vkbInstance, m_surface);
        vkb::destroy_instance(m_vkbInstance);

        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
    }

private:
    // Vulkan Handles
    vk::Instance m_instance;
    vk::Device m_device;
    vk::SurfaceKHR m_surface;
    vk::Queue m_graphicsQueue, m_presentQueue;
    vk::RenderPass m_renderPass;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
    vk::CommandPool m_commandPool;
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

    // Wagoog
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

int main() {
#if defined(WIN32)
    SetConsoleMode(console, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

   App app;

   app.init();

   app.windowLoop();

   app.destroy();

    return 0;
}