#include "Renderer.h"

#include "ChopperImGui.h"
#include "Descriptors.h"
#include "Core/Mesh.h"
#include "GLFW/glfw3.h"


void Renderer::init()
{
    if (enableValidationLayers) printf("Renderer Validation Layers ON\n");

    window_.windowInit(*this);

    createInstance();
    setupDebugMessenger();
    createSurface();

    device_.init(instance_, surface_, queue_index_, queue);

    msaa_samples_ = device_.getMaxUsableSampleCount();

    createAllocator();

    swapchain_.init(*this);
    pipeline_.init(*this);

    createCommandPool();

    // --------------------
    // Creating Resources
    // --------------------
    // ->Color and Depth for Swapchain
    Resources::createColorResources(color_image, *this);
    Resources::createDepthResources(depth_image, *this);

    // Model Texture
    Resources::createTextureImage(model_texture, "testmodels/hercules_kalliope/T_Herkules_Kalliope.png", *this);
    Resources::createTextureImageView(model_texture, *this);
    model_texture.createTextureSampler(sampler_model_texture, *this);

    // Camera init
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_.window_, &width, &height);
    camera_.init(window_.window_, width, height);

    mesh.loadModel("testmodels/hercules_kalliope/hercules_kalliope.obj", mesh);

    // App stuff...
    // -> TODO: This should be done dynamically and automatically :/

    // Object 1 - Center
    gameObjects[0].position = {0.0f, 0.0f, 0.0f};
    gameObjects[0].rotation = {0.0f, 0.0f, 0.0f};
    gameObjects[0].scale = {1.0f, 1.0f, 1.0f};

    // Object 2 - Left
    gameObjects[1].position = {0.0f, 0.0f, 0.0f};
    gameObjects[1].rotation = {0.0f, glm::radians(45.0f), 0.0f};
    gameObjects[1].scale = {1.0f, 1.0f, 1.0f};

    // Object 3 - Right
    gameObjects[2].position = {0.0f, 0.0f, 0.0f};
    gameObjects[2].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
    gameObjects[2].scale = {1.0f, 1.0f, 1.0f};

    mesh.createVertexBuffer(mesh, *this);
    mesh.createIndexBuffer(mesh, *this);

    for (auto& game_object : gameObjects)
    {
        Resources::createUniformBuffers(game_object, allocator_);
    }

    // We need MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT descriptor sets
    int descriptor_count = 3 * MAX_FRAMES_IN_FLIGHT;
    std::array pool_sizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descriptor_count),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descriptor_count)
    };
    Descriptors::createDescriptorPool(descriptor_pool, pool_sizes, descriptor_count, *this);


    for (auto& gameObject : gameObjects)
    {
        Descriptors::createDescriptorSets(gameObject, descriptor_pool, model_texture.image_base_, sampler_model_texture,
                                          *this);
    }
    createCommandBuffers();
    createSyncObjects();
    
    ChopperImGui::initImgui(imgui_descriptor_pool, *this);
    
    printf("Renderer Initialized!\n");
}

void Renderer::mainLoop(double dt)
{
    
    glfwPollEvents();
    camera_.enabled_ = (!imgui_io->WantCaptureMouse); 
    camera_.update(dt);
    drawFrame();
}

void Renderer::drawFrame()
{
    
    while (vk::Result::eTimeout == device_.device_.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));
    if (window_.framebuffer_resized_)
    {
        window_.framebuffer_resized_ = false;
        swapchain_.recreateSwapChain(color_image, depth_image);
        return;
    }
    auto [result, imageIndex] = swapchain_.swapchain_.acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphore[semaphoreIndex], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        swapchain_.recreateSwapChain(color_image, depth_image);
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);

    ChopperImGui::imgui_endFrame();

    device_.device_.resetFences(*inFlightFences[currentFrame]);
    command_buffers_[currentFrame].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &*presentCompleteSemaphore[semaphoreIndex];
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*command_buffers_[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &*renderFinishedSemaphore[imageIndex];

    queue.submit(submitInfo, *inFlightFences[currentFrame]);

    vk::PresentInfoKHR presentInfoKHR{};
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores = &*renderFinishedSemaphore[imageIndex];
    presentInfoKHR.swapchainCount = 1;
    presentInfoKHR.pSwapchains = &*swapchain_.swapchain_;
    presentInfoKHR.pImageIndices = &imageIndex;

    result = queue.presentKHR(presentInfoKHR);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || window_.
        framebuffer_resized_)
    {
        window_.framebuffer_resized_ = false;
        swapchain_.recreateSwapChain(color_image, depth_image);
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
Renderer::~Renderer()
{
    //cleanUp();
    //delete pipeline_;
    //delete swapchain_;
    //delete window_;
    //delete this;
}
*/

void Renderer::cleanUp()
{
    device_.device_.waitIdle();

    ChopperBuffer::cleanMesh(mesh, *this);
    ChopperBuffer::cleanImage(color_image, *this);
    ChopperBuffer::cleanImage(depth_image, *this);
    ChopperBuffer::cleanTextureImage(model_texture, *this);
    for (auto& gameObject : gameObjects)
        ChopperBuffer::cleanGameObject(gameObject, *this);

    vmaDestroyAllocator(allocator_);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    swapchain_.cleanupSwapChain();
    glfwDestroyWindow(window_.window_);
    glfwTerminate();
}


void Renderer::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    glm::mat4 view = camera_.getView();
    glm::mat4 proj = camera_.getProj();

    for (auto& gameObject : gameObjects)
    {
        gameObject.rotation.y += 0.000f; // Slow rotation around Y axis
        glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 model = gameObject.getModelMatrix() * initialRotation;
        UBO_MVP ubo{
            .model = model,
            .view = view,
            .proj = proj
        };
        memcpy(gameObject.uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    command_buffers_[currentFrame].begin({});
    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    swapchain_.transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite, // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
    );

    // Transition the multisampled color image to COLOR_ATTACHMENT_OPTIMAL
    color_image.customTransitionImageLayout(
        color_image.image_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor,
        *this
    );

    // Transition the depth image to DEPTH_ATTACHMENT_OPTIMAL
    depth_image.customTransitionImageLayout(
        depth_image.image_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
        vk::ImageAspectFlagBits::eDepth,
        *this
    );
    // Transition depth image to depth attachment optimal layout
    vk::ImageMemoryBarrier2 depthBarrier = {
        vk::PipelineStageFlagBits2::eTopOfPipe,
        {},
        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
        vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentRead |
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        depth_image.image_,
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eDepth,
            0,
            1,
            0,
            1
        }
    };
    vk::DependencyInfo depthDependencyInfo = {};
    depthDependencyInfo.dependencyFlags = {};
    depthDependencyInfo.imageMemoryBarrierCount = 1;
    depthDependencyInfo.pImageMemoryBarriers = &depthBarrier;

    command_buffers_[currentFrame].pipelineBarrier2(depthDependencyInfo);

    vk::ClearValue clearColor = vk::ClearColorValue(0.03f, 0.03f, 0.033f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    // Color attachment (multisampled) with resolve attachment
    vk::RenderingAttachmentInfo colorAttachment = {};
    colorAttachment.imageView = color_image.image_view_;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage;
    colorAttachment.resolveImageView = swapchain_.swapchain_image_views_[imageIndex];
    colorAttachment.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.clearValue = clearColor;


    // Depth attachment
    vk::RenderingAttachmentInfo depthAttachment = {};
    depthAttachment.imageView = depth_image.image_view_;
    depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.clearValue = clearDepth;


    vk::RenderingInfo renderingInfo = {};
    renderingInfo.renderArea.offset.setX(0);
    renderingInfo.renderArea.offset.setY(0);
    renderingInfo.renderArea.extent = swapchain_.swapchain_extent_;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;


    command_buffers_[currentFrame].beginRendering(renderingInfo);
    command_buffers_[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_.graphics_pipeline_);
    command_buffers_[currentFrame].setViewport(0, vk::Viewport(
                                                   0.0f, 0.0f, static_cast<float>(swapchain_.swapchain_extent_.width),
                                                   static_cast<float>(swapchain_.swapchain_extent_.height), 0.0f,
                                                   1.0f));
    command_buffers_[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain_.swapchain_extent_));
    // Bind vertex and index buffers
    command_buffers_[currentFrame].bindVertexBuffers(0, vk::Buffer(mesh.vertex_buffer_), {0});
    command_buffers_[currentFrame].bindIndexBuffer(mesh.index_buffer_, 0, vk::IndexType::eUint32);


    // Draw each object with its own descriptor set
    for (const auto& gameObject : gameObjects)
    {
        // Bind the descriptor set for this object
        command_buffers_[currentFrame].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            *pipeline_.pipeline_layout_,
            0,
            *gameObject.descriptorSets[currentFrame],
            nullptr
        );

        // Draw the object
        command_buffers_[currentFrame].drawIndexed(mesh.indices_.size(), 1, 0, 0, 0);
    }

    // ImGui!
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffers_[currentFrame]);

    command_buffers_[currentFrame].endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    swapchain_.transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
        {}, // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe // dstStage
    );
    command_buffers_[currentFrame].end();
}


void Renderer::createInstance()
{
    vk::ApplicationInfo appInfo{};

    appInfo.pApplicationName = "Chopper Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers)
    {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = context_.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer)
    {
        return std::ranges::none_of(layerProperties,
                                    [requiredLayer](auto const& layerProperty)
                                    {
                                        return strcmp(layerProperty.layerName, requiredLayer) == 0;
                                    });
    }))
    {
        throw std::runtime_error("One or more required layers are not supported!");
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context_.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions)
    {
        if (std::ranges::none_of(extensionProperties,
                                 [requiredExtension](auto const& extensionProperty)
                                 {
                                     return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                                 }))
        {
            throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
        }
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    instance_ = vk::raii::Instance(context_, createInfo);
}

void Renderer::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;

    debug_messenger_ = instance_.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Renderer::createSurface()
{
    VkSurfaceKHR _surface;

    if (glfwCreateWindowSurface(*instance_, window_.window_, nullptr, &_surface) != 0)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface_ = vk::raii::SurfaceKHR(instance_, _surface);
}

void Renderer::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = queue_index_;

    command_pool = vk::raii::CommandPool(device_.device_, poolInfo);
}

void Renderer::createCommandBuffers()
{
    command_buffers_.clear();

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = command_pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    command_buffers_ = vk::raii::CommandBuffers(device_.device_, allocInfo);
}

void Renderer::createSyncObjects()
{
    presentCompleteSemaphore.clear();
    renderFinishedSemaphore.clear();
    inFlightFences.clear();

    for (size_t i = 0; i < swapchain_.swapchain_images_.size(); i++)
    {
        presentCompleteSemaphore.emplace_back(device_.device_, vk::SemaphoreCreateInfo());
        renderFinishedSemaphore.emplace_back(device_.device_, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        inFlightFences.emplace_back(device_.device_, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void Renderer::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = *device_.physical_device_;
    allocatorInfo.device = *device_.device_;
    allocatorInfo.instance = *instance_;

    vmaCreateAllocator(&allocatorInfo, &allocator_);
}


std::vector<const char*> Renderer::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) extensions.push_back(vk::EXTDebugUtilsExtensionName);

    return extensions;
}
