#include "HelloTriangle.h"

template <>
struct std::hash<Chopper::Vertex>
{
    size_t operator()(Chopper::Vertex const& vertex) const noexcept
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<
            glm::vec2>()(vertex.texCoord) << 1);
    }
};

namespace Chopper
{
    void HelloTriangleApplication::run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    void HelloTriangleApplication::initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        monitors = glfwGetMonitors(&monitors_count);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Chopper Engine", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetWindowPosCallback(window, framebufferResizeCallback);

        // load chopper icon
        GLFWimage images[1];
        int width, height, channels;

        unsigned char* pixels = stbi_load("../core/resources/icons/chopper.png", &width, &height, &channels, 4);
        if (pixels)
        {
            images[0].width = width;
            images[0].height = height;
            images[0].pixels = pixels;
            glfwSetWindowIcon(window, 1, images);
            stbi_image_free(pixels);
        }
        else std::cerr << "Failed to load window icon!" << std::endl;
    }


    void HelloTriangleApplication::mainLoop()
    {
        initImGui();
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }
        device.waitIdle();
    }


    void HelloTriangleApplication::initVulkan()
    {
        if (enableValidationLayers) printf("Validation Layers ON\n");
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        msaaSamples = getMaxUsableSampleCount();
        createLogicalDevice();
        createAllocator(*instance, *physicalDevice, *device);
        createSwapChain();
        createImageViews();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createColorResources();
        createDepthResources();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void HelloTriangleApplication::cleanupSwapChain()
    {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void HelloTriangleApplication::vmaCleanup()
    {
        vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
        vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
        vmaDestroyImage(allocator, colorImage, colorImageAllocation);
        vmaDestroyImage(allocator, depthImage, depthImageAllocation);
        vmaDestroyImage(allocator, textureImage, textureImageAllocation);
        for (int i = 0; i < uniformBuffers.size(); ++i)
        {
            vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBuffersAllocation[i]);
        }
        vmaDestroyAllocator(allocator);
    }

    void HelloTriangleApplication::cleanup()
    {
        device.waitIdle();

        vmaCleanup();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        cleanupSwapChain();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void HelloTriangleApplication::recreateSwapChain()
    {
        //printf("Recreating Swapchain...\n");
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        vmaDestroyImage(allocator, colorImage, colorImageAllocation);
        vmaDestroyImage(allocator, depthImage, depthImageAllocation);
        
        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
    }

    void HelloTriangleApplication::createInstance()
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
        auto layerProperties = context.enumerateInstanceLayerProperties();
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
        auto extensionProperties = context.enumerateInstanceExtensionProperties();
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

        instance = vk::raii::Instance(context, createInfo);
    }

    void HelloTriangleApplication::pickPhysicalDevice()
    {
        std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

        const auto devIter = std::ranges::find_if(
            devices,
            [&](auto const& device)
            {
                // Check if the device supports the Vulkan 1.3 API version
                bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

                // Check if any of the queue families support graphics operations
                auto queueFamilies = device.getQueueFamilyProperties();
                bool supportsGraphics =
                    std::ranges::any_of(queueFamilies, [](auto const& qfp)
                    {
                        return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
                    });

                // Check if all required device extensions are available
                auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();

                bool supportsAllRequiredExtensions = std::ranges::all_of(requiredDeviceExtension,
                                                                         [&availableDeviceExtensions](
                                                                         auto const& requiredDeviceExtension)
                                                                         {
                                                                             return std::ranges::any_of(
                                                                                 availableDeviceExtensions,
                                                                                 [requiredDeviceExtension](
                                                                                 auto const& availableDeviceExtension)
                                                                                 {
                                                                                     return strcmp(
                                                                                         availableDeviceExtension.
                                                                                         extensionName,
                                                                                         requiredDeviceExtension) == 0;
                                                                                 });
                                                                         });

                auto features = device.template getFeatures2<
                    vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
                bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.
                                                         samplerAnisotropy &&
                    features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                    features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

                return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions &&
                    supportsRequiredFeatures;
            });

        if (devIter != devices.end()) physicalDevice = *devIter;
        else throw std::runtime_error("failed to find a suitable GPU!");
    }

    void HelloTriangleApplication::createLogicalDevice()
    {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports both graphics and present
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
            {
                // found a queue family that supports both graphics and present
                queueIndex = qfpIndex;
                break;
            }
        }
        if (queueIndex == ~0)
        {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        // query for Vulkan 1.3 features
        //vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        //    {},                                                 // vk::PhysicalDeviceFeatures2
        //    {.dynamicRendering = true },      // vk::PhysicalDeviceVulkan13Features
        //    {.extendedDynamicState = true }                     // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        //};

        vk::PhysicalDeviceFeatures2 physical_device_features2{};
        physical_device_features2.features.samplerAnisotropy = VK_TRUE;

        vk::PhysicalDeviceDynamicRenderingFeaturesKHR device_dynamic_rendering_features{};
        device_dynamic_rendering_features.dynamicRendering = VK_TRUE;
        device_dynamic_rendering_features.sType = vk::PhysicalDeviceDynamicRenderingFeaturesKHR::structureType;


        vk::PhysicalDeviceVulkan13Features vulkan_13_features{};
        vulkan_13_features.sType = vk::PhysicalDeviceVulkan13Features::structureType;
        vulkan_13_features.dynamicRendering = VK_TRUE;
        vulkan_13_features.synchronization2 = VK_TRUE;

        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT dynamic_state_features{};
        dynamic_state_features.extendedDynamicState = VK_TRUE;

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain(
            physical_device_features2,
            vulkan_13_features,
            dynamic_state_features
        );

        // create a Device
        float queuePriority = 0.0f;

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
        deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        vk::DeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = vk::DeviceCreateInfo::structureType;
        deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);

        queue = vk::raii::Queue(device, queueIndex, 0);
    }

    void HelloTriangleApplication::setupDebugMessenger()
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

        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void HelloTriangleApplication::createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice,
                                                   VkDevice device)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;

        vmaCreateAllocator(&allocatorInfo, &allocator);
    }

    void HelloTriangleApplication::createSurface()
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    void HelloTriangleApplication::createSwapChain()
    {
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        swapChainImageFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
        swapChainExtent = chooseSwapExtent(surfaceCapabilities);
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
                            ? surfaceCapabilities.maxImageCount
                            : minImageCount;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
        swapChainCreateInfo.surface = surface;
        swapChainCreateInfo.minImageCount = minImageCount;
        swapChainCreateInfo.imageFormat = swapChainImageFormat;
        swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        swapChainCreateInfo.imageExtent = swapChainExtent;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(surface));
        swapChainCreateInfo.clipped = true;

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
        //printf("Swapchain created!\n");
    }

    void HelloTriangleApplication::createImageViews()
    {
        vk::ImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = swapChainImageFormat;
        imageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        for (auto image : swapChainImages)
        {
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }

    void HelloTriangleApplication::createDescriptorSetLayout()
    {
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex,
                                           nullptr),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
                                           vk::ShaderStageFlagBits::eFragment, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
    }

    void HelloTriangleApplication::createGraphicsPipeline()
    {
        vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = shaderModule;
        vertShaderStageInfo.pName = "vertMain";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = shaderModule;
        fragShaderStageInfo.pName = "fragMain";

        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable = vk::False;
        rasterizer.rasterizerDiscardEnable = vk::False;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = vk::False;
        rasterizer.depthBiasSlopeFactor = 1.0f;
        rasterizer.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.sampleShadingEnable = vk::False;

        vk::PipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.depthTestEnable = vk::True;
        depthStencil.depthWriteEnable = vk::True;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = vk::False;
        depthStencil.stencilTestEnable = vk::False;


        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = vk::False;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.logicOpEnable = vk::False;
        colorBlending.logicOp = vk::LogicOp::eCopy;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        vk::Format depthFormat = findDepthFormat();
        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat;
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;


        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = nullptr;


        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }

    void HelloTriangleApplication::createCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueIndex;

        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    void HelloTriangleApplication::createColorResources()
    {
        vk::Format colorFormat = swapChainImageFormat;

        createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        1,
        msaaSamples,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        colorImage,                // VkImage
        colorImageAllocation       // VmaAllocation
    );
        colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
    }

    void HelloTriangleApplication::createDepthResources()
    {
        vk::Format depthFormat = findDepthFormat();

        createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        1,
        msaaSamples,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        depthImage,               // VkImage
        depthImageAllocation      // VmaAllocation
    );
        depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

        //transitionImageLayout(depthImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    }

    vk::Format HelloTriangleApplication::findSupportedFormat(const std::vector<vk::Format>& candidates,
                                                             vk::ImageTiling tiling, vk::FormatFeatureFlags features)
    {
        auto formatIt = std::ranges::find_if(candidates, [&](auto const format)
        {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);
            return (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
                ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)));
        });
        if (formatIt == candidates.end())
        {
            throw std::runtime_error("failed to find supported format!");
        }
        return *formatIt;
    }

    vk::Format HelloTriangleApplication::findDepthFormat()
    {
        return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    bool HelloTriangleApplication::hasStencilComponent(vk::Format format)
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    void HelloTriangleApplication::createTextureImage()
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    // ---------------------------
    // 1. Create staging buffer (CPU visible)
    // ---------------------------
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocInfo{};

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT; // so we can memcpy directly

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create staging buffer for texture!");
    }

    // Copy pixels into mapped memory
    memcpy(stagingAllocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

    stbi_image_free(pixels);

    // ---------------------------
    // 2. Create GPU-only texture image with mip levels
    // ---------------------------
    createImage(
        texWidth,
        texHeight,
        mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |    // needed for mipmap generation
        vk::ImageUsageFlagBits::eTransferDst |    // we copy into it from staging
        vk::ImageUsageFlagBits::eSampled,
        textureImage,                             // VkImage
        textureImageAllocation                    // VmaAllocation
    );

    // ---------------------------
    // 3. Transition, copy, generate mipmaps
    // ---------------------------
    transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);

    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);

    // ---------------------------
    // 4. Cleanup staging
    // ---------------------------
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}


    void HelloTriangleApplication::generateMipmaps(VkImage& image, vk::Format imageFormat, int32_t texWidth,
                                                   int32_t texHeight, uint32_t mipLevels)
    {
        // Check if image format supports linear blit-ing
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier = {};
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.image = image;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                                           {}, {}, {}, barrier);

            vk::ArrayWrapper1D<vk::Offset3D, 2> offsets, dstOffsets;
            offsets[0] = vk::Offset3D(0, 0, 0);
            offsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            dstOffsets[0] = vk::Offset3D(0, 0, 0);
            dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

            vk::ImageBlit blit = {};
            //blit.srcSubresource = {};
            blit.srcOffsets = offsets;
            //blit.dstSubresource = {};
            blit.dstOffsets = dstOffsets;

            blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
            blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);

            commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                                     vk::ImageLayout::eTransferDstOptimal, {blit}, vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                           vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                       {}, {}, {}, barrier);

        endSingleTimeCommands(*commandBuffer);
    }

    vk::SampleCountFlagBits HelloTriangleApplication::getMaxUsableSampleCount()
    {
        vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

        vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
            physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

        return vk::SampleCountFlagBits::e1;
    }


    void HelloTriangleApplication::createTextureImageView()
    {
        textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor,
                                           mipLevels);
    }

    void HelloTriangleApplication::createTextureSampler()
    {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = vk::True;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.compareEnable = vk::False;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        //samplerInfo.minLod = 0;
        //amplerInfo.maxLod = 8;
        textureSampler = vk::raii::Sampler(device, samplerInfo);
    }

    vk::raii::ImageView HelloTriangleApplication::createImageView(const VkImage& image, vk::Format format,
                                                                  vk::ImageAspectFlags aspectFlags,
                                                                  uint32_t mipLevels) const
    {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {aspectFlags, 0, mipLevels, 0, 1};
        return vk::raii::ImageView(device, viewInfo);
    }

    void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                                           vk::SampleCountFlagBits numSamples, vk::Format format,
                                           vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                           VkImage& image, VmaAllocation& imageAllocation)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = static_cast<VkFormat>(format);
    imageInfo.tiling = static_cast<VkImageTiling>(tiling);
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(numSamples);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;  // VRAM preferred
    // For GPU-only images (textures, depth, color attachments), device-local is usually best.

    if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image with VMA!");
    }
}


    void HelloTriangleApplication::transitionImageLayout(const VkImage& image, const vk::ImageLayout oldLayout,
                                                         const vk::ImageLayout newLayout, uint32_t mipLevels)
    {
        auto commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.image = image;
        barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1};

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout ==
            vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }
        commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
        endSingleTimeCommands(*commandBuffer);
    }

    void HelloTriangleApplication::copyBufferToImage(const VkBuffer& buffer, VkImage& image,
                                                     uint32_t width, uint32_t height)
    {
        std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands();
        vk::BufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{width, height, 1};

        commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
        endSingleTimeCommands(*commandBuffer);
    }

    std::unique_ptr<vk::raii::CommandBuffer> HelloTriangleApplication::beginSingleTimeCommands()
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(
            std::move(vk::raii::CommandBuffers(device, allocInfo).front()));

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer->begin(beginInfo);

        return commandBuffer;
    }

    void HelloTriangleApplication::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

    void HelloTriangleApplication::createCommandBuffers()
    {
        commandBuffers.clear();

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void HelloTriangleApplication::recordCommandBuffer(uint32_t imageIndex)
    {
        commandBuffers[currentFrame].begin({});
        // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {}, // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite, // dstAccessMask
            vk::PipelineStageFlagBits2::eTopOfPipe, // srcStage
            vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
        );

        // Transition the multisampled color image to COLOR_ATTACHMENT_OPTIMAL
        transition_image_layout_custom(
            colorImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
        );

        // Transition the depth image to DEPTH_ATTACHMENT_OPTIMAL
        transition_image_layout_custom(
            depthImage,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests,
            vk::ImageAspectFlagBits::eDepth
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
            depthImage,
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

        commandBuffers[currentFrame].pipelineBarrier2(depthDependencyInfo);

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

        // Color attachment (multisampled) with resolve attachment
        vk::RenderingAttachmentInfo colorAttachment = {};
        colorAttachment.imageView = colorImageView;
        colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage;
        colorAttachment.resolveImageView = swapChainImageViews[imageIndex];
        colorAttachment.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.clearValue = clearColor;


        // Depth attachment
        vk::RenderingAttachmentInfo depthAttachment = {};
        depthAttachment.imageView = depthImageView;
        depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.clearValue = clearDepth;


        vk::RenderingInfo renderingInfo = {};
        renderingInfo.renderArea.offset.setX(0);
        renderingInfo.renderArea.offset.setY(0);
        renderingInfo.renderArea.extent = swapChainExtent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        /*
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
            depthImage,
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

        commandBuffers[currentFrame].pipelineBarrier2(depthDependencyInfo);

        vk::ClearValue clearColor = vk::ClearColorValue(0.01f, 0.01f, 0.01f, 1.0f);
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

        vk::RenderingAttachmentInfo colorAttachmentInfo = {};
        colorAttachmentInfo.imageView = swapChainImageViews[imageIndex];
        colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachmentInfo.clearValue = clearColor;

        vk::RenderingAttachmentInfo depthAttachmentInfo = {};
        depthAttachmentInfo.imageView = depthImageView;
        depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachmentInfo.clearValue = clearDepth;

        vk::RenderingInfo renderingInfo = {};
        renderingInfo.renderArea.offset.setX(0);
        renderingInfo.renderArea.offset.setY(0);
        renderingInfo.renderArea.extent = swapChainExtent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachmentInfo;
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    */
        commandBuffers[currentFrame].beginRendering(renderingInfo);
        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                                                                 static_cast<float>(swapChainExtent.height), 0.0f,
                                                                 1.0f));
        commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
        commandBuffers[currentFrame].bindVertexBuffers(0, vk::Buffer(vertexBuffer), {0});
        commandBuffers[currentFrame].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                                        *descriptorSets[currentFrame], nullptr);
        commandBuffers[currentFrame].drawIndexed(indices.size(), 1, 0, 0, 0);

        // ImGui!
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffers[currentFrame]);

        commandBuffers[currentFrame].endRendering();

        // After rendering, transition the swapchain image to PRESENT_SRC
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
            {}, // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe // dstStage
        );
        commandBuffers[currentFrame].end();
    }

    void HelloTriangleApplication::transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    )
    {
        vk::ImageMemoryBarrier2 barrier = {};
        barrier.srcStageMask = src_stage_mask;
        barrier.srcAccessMask = src_access_mask;
        barrier.dstStageMask = dst_stage_mask;
        barrier.dstAccessMask = dst_access_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = swapChainImages[imageIndex];
        //barrier.subresourceRange = {};
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::DependencyInfo dependency_info = {};
        dependency_info.dependencyFlags = {};
        dependency_info.imageMemoryBarrierCount = 1;
        dependency_info.pImageMemoryBarriers = &barrier;

        commandBuffers[currentFrame].pipelineBarrier2(dependency_info);
    }

    void HelloTriangleApplication::loadModel()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
        {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
                /*
*/
                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex))
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }


    void HelloTriangleApplication::createVertexBuffer()
    {
        //-------------------------------------------------------- Old way
        /*
        
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        
        // 1. Create staging buffer
        vk::raii::Buffer stagingBuffer({});

        vk::raii::DeviceMemory stagingBufferMemory({});
        
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
        // Copy vertex data
        memcpy(dataStaging, vertices.data(), bufferSize);
        stagingBufferMemory.unmapMemory();
        // 2. Create GPU-local vertex buffer

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

        // 3. Copy staging → vertex buffer
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        */

        //-------------------------------------------------------- VMA way underneath

        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // 1. Create staging buffer (CPU-visible, for uploading data)
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;

        VkBufferCreateInfo stagingInfo = {};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = bufferSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT; // CPU write access

        VmaAllocationInfo stagingAllocDetails;
        vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, &stagingAllocDetails);

        // Copy vertex data
        memcpy(stagingAllocDetails.pMappedData, vertices.data(), (size_t)bufferSize);

        // 2. Create GPU-local vertex buffer
        VkBufferCreateInfo vertexInfo = {};
        vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexInfo.size = bufferSize;
        vertexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo vertexAllocInfo = {};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // Let VMA choose VRAM

        vmaCreateBuffer(allocator, &vertexInfo, &vertexAllocInfo,
                        &vertexBuffer, &vertexBufferAllocation, nullptr);

        // 3. Copy staging → vertex buffer
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // 4. Cleanup staging
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
    }

    void HelloTriangleApplication::copyBuffer(VkBuffer srcBuffer,
                                              VkBuffer dstBuffer,
                                              VkDeviceSize size)
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
        commandCopyBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
        commandCopyBuffer.end();
        vk::SubmitInfo submit_info = {};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &*commandCopyBuffer;
        queue.submit(submit_info, nullptr);
        queue.waitIdle();
    }

    void HelloTriangleApplication::createIndexBuffer()
    {
        // 1. Create staging buffer (CPU-visible, for uploading data)
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;

        VkBufferCreateInfo stagingInfo = {};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = bufferSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.pNext = nullptr;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT; // CPU write access

        VmaAllocationInfo stagingAllocDetails;
        vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, &stagingAllocDetails);

        // Copy vertex data
        memcpy(stagingAllocDetails.pMappedData, indices.data(), (size_t)bufferSize);

        // 2. Create GPU-local vertex buffer
        VkBufferCreateInfo indexInfo = {};
        indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexInfo.size = bufferSize;
        indexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocInfo = {};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // Let VMA choose VRAM

        vmaCreateBuffer(allocator, &indexInfo, &indexAllocInfo,
                        &indexBuffer, &indexBufferAllocation, nullptr);

        // 3. Copy staging → index buffer
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        // 4. Cleanup staging
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

        /*
                // 1. Create staging buffer
                vk::raii::Buffer stagingBuffer({});
                vk::raii::DeviceMemory stagingBufferMemory({});
                
                createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                             stagingBuffer, stagingBufferMemory);
        
                void* data = stagingBufferMemory.mapMemory(0, bufferSize);
                // Copy vertex data
                memcpy(data, indices.data(), (size_t)bufferSize);
                stagingBufferMemory.unmapMemory();
        // 2. Create GPU-local vertex buffer
                createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                             vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
        // 3. Copy staging → vertex buffer
                copyBuffer(*stagingBuffer, *indexBuffer, bufferSize);
        */
    }
void HelloTriangleApplication::createUniformBuffers()
{
    uniformBuffers.clear();
    uniformBuffersAllocation.clear();
    uniformBuffersMapped.clear();

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;  // keep it mapped

        VmaAllocationInfo vmaAllocDetails{};
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                            &uniformBuffers[i], &uniformBuffersAllocation[i], &vmaAllocDetails) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create uniform buffer with VMA!");
        }

        // Already mapped because of the flag
        uniformBuffersMapped[i] = vmaAllocDetails.pMappedData;
    }
}


    void HelloTriangleApplication::createDescriptorPool()
    {
        std::array poolSize{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT),
        };
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
        poolInfo.pPoolSizes = poolSize.data();

        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void HelloTriangleApplication::createDescriptorSets()
    {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.clear();
        descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.sampler = textureSampler;
            imageInfo.imageView = textureImageView;
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            vk::WriteDescriptorSet descriptor_set0 = {};
            descriptor_set0.dstSet = descriptorSets[i];
            descriptor_set0.dstBinding = 0;
            descriptor_set0.dstArrayElement = 0;
            descriptor_set0.descriptorCount = 1;
            descriptor_set0.descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptor_set0.pBufferInfo = &bufferInfo;


            vk::WriteDescriptorSet descriptor_set1 = {};
            descriptor_set1.dstSet = descriptorSets[i];
            descriptor_set1.dstBinding = 1;
            descriptor_set1.dstArrayElement = 0;
            descriptor_set1.descriptorCount = 1;
            descriptor_set1.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptor_set1.pImageInfo = &imageInfo;

            std::array descriptorWrites{
                vk::WriteDescriptorSet{
                    descriptor_set0
                },
                vk::WriteDescriptorSet{
                    descriptor_set1
                }
            };
            device.updateDescriptorSets(descriptorWrites, {});
        }
    }

    void HelloTriangleApplication::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                                vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer,
                                                vk::raii::DeviceMemory& bufferMemory)
    {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(bufferMemory, 0);
    }

    uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void HelloTriangleApplication::transition_image_layout_custom(
        VkImage& image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags aspect_mask
    )
    {
        vk::ImageMemoryBarrier2 barrier = {};
        barrier.srcStageMask = src_stage_mask;
        barrier.srcAccessMask = src_access_mask;
        barrier.dstStageMask = dst_stage_mask;
        barrier.dstAccessMask = dst_access_mask;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {
            aspect_mask,
            0,
            1,
            0,
            1
        };

        vk::DependencyInfo dependency_info = {};
        dependency_info.dependencyFlags = {};
        dependency_info.imageMemoryBarrierCount = 1;
        dependency_info.pImageMemoryBarriers = &barrier;

        commandBuffers[currentFrame].pipelineBarrier2(dependency_info);
    }

    void HelloTriangleApplication::createSyncObjects()
    {
        presentCompleteSemaphore.clear();
        renderFinishedSemaphore.clear();
        inFlightFences.clear();

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
            renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            inFlightFences.emplace_back(device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
        }
    }

    void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        UniformBufferObject ubo{
            rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.8f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::perspective(glm::radians(40.0f),
                             static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.
                                 height), 0.1f, 10.0f)
        };
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void HelloTriangleApplication::drawFrame()
    {
        while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));
        if (framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }
        auto [result, imageIndex] = swapChain.acquireNextImage(
            UINT64_MAX, *presentCompleteSemaphore[semaphoreIndex], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        paintImGui();

        device.resetFences(*inFlightFences[currentFrame]);
        commandBuffers[currentFrame].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

        vk::SubmitInfo submitInfo{};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &*presentCompleteSemaphore[semaphoreIndex];
        submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &*renderFinishedSemaphore[imageIndex];

        queue.submit(submitInfo, *inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfoKHR{};
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores = &*renderFinishedSemaphore[imageIndex];
        presentInfoKHR.swapchainCount = 1;
        presentInfoKHR.pSwapchains = &*swapChain;
        presentInfoKHR.pImageIndices = &imageIndex;

        result = queue.presentKHR(presentInfoKHR);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void HelloTriangleApplication::initImGui()
    {
        // Create Descriptor Pool
        // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
        VkDescriptorPoolSize pool_sizes[] =
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        //for (VkDescriptorPoolSize& pool_size : pool_sizes)
        //    pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        imgui_descriptor_pool = vk::raii::DescriptorPool(device, pool_info);

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        //(void)io;
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        ImGui::GetStyle().FontScaleMain = 1.3f;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = vk::ApiVersion14;
        // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
        init_info.Instance = *instance;
        init_info.PhysicalDevice = *physicalDevice;
        init_info.Device = *device;
        init_info.QueueFamily = queueIndex;
        init_info.Queue = *queue;
        init_info.DescriptorPool = *imgui_descriptor_pool;
        init_info.DescriptorPoolSize = 0;
        init_info.RenderPass = nullptr;
        init_info.MinImageCount = 2;
        init_info.ImageCount = 2;
        init_info.MSAASamples = VkSampleCountFlagBits(msaaSamples);
        init_info.UseDynamicRendering = true;
        init_info.Subpass = 0;

        init_info.Allocator = nullptr;
        //init_info.CheckVkResultFn = check_vk_result;

        init_info.PipelineRenderingCreateInfo = {};
        init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        init_info.PipelineRenderingCreateInfo.pNext = nullptr;
        init_info.PipelineRenderingCreateInfo.viewMask = 0;
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        VkFormat format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
        init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        init_info.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        ImGui_ImplVulkan_Init(&init_info);

        // Load Fonts
        //style.FontSizeBase = 20.0f;
        ImFont* font = io->Fonts->AddFontFromFileTTF("../core/resources/fonts/NunitoSans.ttf");
        IM_ASSERT(font != nullptr);
        //io->Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
        //io->Fonts->AddFontDefault();
        //io->Fonts->Build();
        //ImGui::GetStyle().FontSizeBase = 20.0f;
    }

    void HelloTriangleApplication::paintImGui()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        //if (show_demo_window)
        //    ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))
                // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            ImGui::End();
        }

/*
        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
*/
        // Rendering
        ImGui::Render();
    }


    std::vector<const char*> HelloTriangleApplication::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) extensions.push_back(vk::EXTDebugUtilsExtensionName);

        return extensions;
    }
}
