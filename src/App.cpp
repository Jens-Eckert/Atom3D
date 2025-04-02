#include "App.hpp"

// Custom Debug Callback
static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                              VkDebugUtilsMessageTypeFlagsEXT type,
                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                              void* pUserData) {
    auto sevStr = vkb::to_string_message_severity(severity);
    auto typeStr = vkb::to_string_message_type(type);

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

    return VK_FALSE;
}

// Wrapper that just uses the debug callback function for pretty printing.
static void logInfo(const std::string& msg) {
    VkDebugUtilsMessengerCallbackDataEXT* data = new VkDebugUtilsMessengerCallbackDataEXT;
    data->pMessage = msg.data();

    debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, 8, data, nullptr);

    data->pMessage = nullptr;
    delete data;
}

static void logError(const std::string& msg) {
    VkDebugUtilsMessengerCallbackDataEXT* data = new VkDebugUtilsMessengerCallbackDataEXT;
    data->pMessage = msg.data();

    debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, 8, data, nullptr);

    data->pMessage = nullptr;
    delete data;
}

static void logWarning(const std::string& msg) {
    VkDebugUtilsMessengerCallbackDataEXT* data = new VkDebugUtilsMessengerCallbackDataEXT;
    data->pMessage = msg.data();

    debugCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, 8, data, nullptr);

    data->pMessage = nullptr;
    delete data;
}

static void glfwFramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

    app->glfwFramebufferResized = true;
}

bool App::init() {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!\n";
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_glfwWindow = glfwCreateWindow(800, 600, "Atom3D", nullptr, nullptr);

    glfwSetWindowUserPointer(m_glfwWindow, this);
    glfwSetFramebufferSizeCallback(m_glfwWindow, glfwFramebufferResizeCallback);

    vkb::InstanceBuilder builder;
    auto instRet = builder
                       .set_app_name("Atom3D")
                       .request_validation_layers()
                       .require_api_version(ATOM3D_VK_VERSION)
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

    vk::PhysicalDeviceVulkan12Features features;

    features.bufferDeviceAddress = true;

    vkb::PhysicalDeviceSelector selector(m_vkbInstance);
    // selector.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    selector.set_required_features_12(features);
    auto physRet = selector.set_surface(m_surface).select();

    if (!physRet) {
        std::cerr << "Failed to select physical device: " << physRet.error().message() << "\n";
        return false;
    }

    m_vkbPD = physRet.value();

    vkb::DeviceBuilder device_builder{m_vkbPD};
    auto deviceRet = device_builder.build();

    if (!deviceRet) {
        std::cerr << "Failed to create Logical Device: " << deviceRet.error().message() << "\n";
        return false;
    }

    m_vkbDevice = deviceRet.value();
    m_device = m_vkbDevice.device;

    vma::AllocatorCreateInfo allocatorInfo;
    allocatorInfo.setPhysicalDevice(m_vkbPD.physical_device)
        .setDevice(m_device)
        .setInstance(m_instance)
        .setFlags(vma::AllocatorCreateFlagBits::eBufferDeviceAddress);

    m_vmaAllocator = vma::createAllocator(allocatorInfo);

    m_vmaBudgets = m_vmaAllocator.getHeapBudgets();

    for (size_t i = 0; i < m_vmaBudgets.size(); i++) {
        logInfo("Budget: " + std::to_string(m_vmaBudgets[i].budget));
    }

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

    setupScene();

    return true;
}

bool App::createSwapchain() {
    vkb::SwapchainBuilder swapchain_builder{m_vkbDevice};

    auto swap_ret = swapchain_builder.build();
    if (!swap_ret) {
        std::cerr << "Error creating swapchain: " << swap_ret.error().message() << "\n";
        return false;
    }

    m_vkbSwapchain = swap_ret.value();
    m_swapchain = m_vkbSwapchain.swapchain;

    return true;
}

bool App::createSwapchainImageViews() {
    m_swapchainImages.clear();
    m_swapchainImageViews.clear();

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

bool App::recreateSwapchain() {
    m_device.waitIdle();

    cleanupSwapchain();

    createSwapchain();
    createSwapchainImageViews();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();

    return true;
}

void App::cleanupSwapchain() {
    for (auto& f : m_framebuffers) {
        m_device.destroyFramebuffer(f);
    }

    m_device.freeCommandBuffers(m_commandPool, m_commandBuffers);
    m_device.destroyCommandPool(m_commandPool);

    m_vkbSwapchain.destroy_image_views(m_swapchainImageViews);

    m_swapchain = VK_NULL_HANDLE;

    vkb::destroy_swapchain(m_vkbSwapchain);
}

bool App::getQueues() {
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

void App::createRenderPass() {
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

std::vector<char> App::loadSPV(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader " << filename << "\n";
        throw std::runtime_error("oop");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buf(fileSize);

    file.seekg(0);
    file.read(buf.data(), fileSize);

    file.close();

    return buf;
}

vk::ShaderModule App::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo info(vk::ShaderModuleCreateFlagBits(),
                                    code.size(),
                                    reinterpret_cast<const uint32_t*>(code.data()));

    return m_device.createShaderModule(info);
}

bool App::createGraphicsPipeline() {
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
    auto binding = Vertex::getBindingDescription();
    auto attrDesc = Vertex::getAttrDesc();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, binding, attrDesc);

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

bool App::createFramebuffers() {
    m_framebuffers.resize(0);
    m_framebuffers.reserve(m_swapchainImageViews.size());

    for (int i = 0; i < m_swapchainImageViews.size(); i++) {
        vk::ImageView iv = m_swapchainImageViews[i];

        vk::FramebufferCreateInfo info;
        info.setRenderPass(m_renderPass)
            .setAttachmentCount(1)
            .setAttachments(iv)
            .setWidth(m_vkbSwapchain.extent.width)
            .setHeight(m_vkbSwapchain.extent.height)
            .setLayers(1);

        m_framebuffers.push_back(m_device.createFramebuffer(info));
    }

    return true;
}

void App::createCommandPool() {
    vk::CommandPoolCreateInfo info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value());

    m_commandPool = m_device.createCommandPool(info);
}

void App::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, m_framebuffers.size() + 1);

    auto tmp = m_device.allocateCommandBuffers(allocInfo);

    m_mainCommandBuffer = tmp[0];
    m_commandBuffers = std::vector<vk::CommandBuffer>(tmp.begin() + 1, tmp.end());

    // This is done in drawFrame() now !!!!!
    // vk::CommandBufferBeginInfo beginInfo;
    // vk::ClearValue clearVal;
    // clearVal.setColor(vk::ClearColorValue(0, 0, 0, 1));

    // for (size_t i = 0; i < m_commandBuffers.size(); i++) {
    //     m_commandBuffers[i].begin(beginInfo);

    //     vk::RenderPassBeginInfo rpInfo(m_renderPass, m_framebuffers[i], {{0, 0}, {m_vkbSwapchain.extent}}, clearVal);

    //     vk::Viewport viewport((0.0), (0.0), m_vkbSwapchain.extent.width, m_vkbSwapchain.extent.height, 0, 1);
    //     vk::Rect2D scissor({0, 0}, m_vkbSwapchain.extent);

    //     m_commandBuffers[i].setViewport(0, viewport);
    //     m_commandBuffers[i].setScissor(0, scissor);

    //     m_commandBuffers[i].beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    //     m_commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

    //     m_commandBuffers[i].draw(3, 1, 0, 0);

    //     m_commandBuffers[i].endRenderPass();

    //     m_commandBuffers[i].end();
    // }
}

void App::createSyncObjects() {
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

void App::destroySyncObjects() {
    for (auto& s : m_imageAvailableSems)
        m_device.destroySemaphore(s);

    for (auto& s : m_renderFinishedSems)
        m_device.destroySemaphore(s);

    for (auto& f : m_inFlightFences)
        m_device.destroyFence(f);
}

void App::recordDrawCommandsScene(vk::CommandBuffer cb, uint32_t image, Scene* scene) {
    vk::RenderPassBeginInfo rpInfo = {};
    rpInfo.renderPass = m_renderPass;
    rpInfo.framebuffer = m_framebuffers[image];
    rpInfo.renderArea.extent = m_vkbSwapchain.extent;

    vk::Viewport viewport((0.0), (0.0), m_vkbSwapchain.extent.width, m_vkbSwapchain.extent.height, 0, 1);
    vk::Rect2D scissor({0, 0}, m_vkbSwapchain.extent);
    m_commandBuffers[image].setViewport(0, viewport);
    m_commandBuffers[image].setScissor(0, scissor);

    vk::ClearValue clearVal;
    clearVal.setColor(vk::ClearColorValue(0, 0, 0, 1));

    rpInfo.setClearValues(clearVal);

    cb.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

    vk::DeviceSize offsets[] = {0};
    vk::Buffer buffers[] = {m_scene.buffer.buffer};
    cb.bindVertexBuffers(0, 1, buffers, offsets);

    cb.draw(3, 1, 0, 0);

    cb.endRenderPass();
}

bool App::drawFrame() {
    auto fenceWait = m_device.waitForFences(m_inFlightFences[m_currentFrame], true, UINT64_MAX);

    auto imageIndex = m_device.acquireNextImageKHR(m_vkbSwapchain.swapchain,
                                                   UINT64_MAX,
                                                   m_imageAvailableSems[m_currentFrame],
                                                   VK_NULL_HANDLE);

    if (imageIndex.result == vk::Result::eErrorOutOfDateKHR) {
        // logInfo("Recreating Swapchain due to outdated images.");
        return recreateSwapchain();
    } else if (imageIndex.result != vk::Result::eSuccess && imageIndex.result != vk::Result::eSuboptimalKHR) {
        std::cerr << "Failed to acquire swapchain image. Error: " << imageIndex.result << "\n";
        return false;
    }

    if (m_imageInFlightFences[imageIndex.value] != VK_NULL_HANDLE) {
        fenceWait = m_device.waitForFences(m_imageInFlightFences[imageIndex.value], true, UINT64_MAX);
    }

    vk::CommandBufferBeginInfo beginInfo;
    vk::CommandBuffer cb = m_commandBuffers[imageIndex.value];

    cb.begin(beginInfo);

    recordDrawCommandsScene(cb, imageIndex.value, &m_scene);

    cb.end();

    m_imageInFlightFences[imageIndex.value] = m_inFlightFences[m_currentFrame];

    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    vk::SubmitInfo subInfo;
    subInfo.setWaitSemaphores(m_imageAvailableSems[m_currentFrame])
        .setWaitDstStageMask(waitStages)
        .setCommandBuffers(cb)
        .setSignalSemaphores(m_renderFinishedSems[m_currentFrame]);

    m_device.resetFences(m_inFlightFences[m_currentFrame]);

    m_graphicsQueue.submit(subInfo, m_inFlightFences[m_currentFrame]);

    // vk::SwapchainKHR tmpswpch = m_vkbSwapchain.swapchain;

    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(m_renderFinishedSems[m_currentFrame])
        .setSwapchains(m_swapchain)
        .setImageIndices(imageIndex.value);

    auto r = m_presentQueue.presentKHR(presentInfo);

    if (r == vk::Result::eErrorOutOfDateKHR ||
        r == vk::Result::eSuboptimalKHR ||
        glfwFramebufferResized) {
        // logInfo("Recreating swapchain after present.");
        glfwFramebufferResized = false;
        recreateSwapchain();
    } else if (r != vk::Result::eSuccess) {
        std::cerr << "Failed to present swapchain image.\n";
        return false;
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

void App::windowLoop() {
    while (!glfwWindowShouldClose(m_glfwWindow)) {
        glfwPollEvents();

        if (!drawFrame()) {
            std::cerr << "Failed to draw frame\n";
            return;
        }
    }
}

void App::destroy() {
    m_device.waitIdle();

    destroySyncObjects();

    cleanupSwapchain();

    // vkb::destroy_swapchain(m_vkbSwapchain);

    m_device.destroyPipeline(m_graphicsPipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);
    m_device.destroyRenderPass(m_renderPass);

    m_vmaAllocator.destroyBuffer(m_scene.buffer.buffer, m_scene.buffer.allocation);
    m_vmaAllocator.destroy();

    m_device = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;

    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_surface(m_vkbInstance, m_surface);
    vkb::destroy_instance(m_vkbInstance);

    glfwDestroyWindow(m_glfwWindow);
    glfwTerminate();
}

void App::setupScene() {
    m_scene.uploadMeshes(m_vmaAllocator, m_mainCommandBuffer, m_graphicsQueue);
}