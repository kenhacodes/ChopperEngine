#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <limits>
#include <array>
#include <chrono>


#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>
#include "vma/vk_mem_alloc.h"

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>

//#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "Camera.h"

namespace Chopper
{
    constexpr uint32_t WIDTH = 1920;
    constexpr uint32_t HEIGHT = 1080;
    constexpr uint64_t FenceTimeout = 100000000;
    const std::string MODEL_PATH = "testmodels/hercules_kalliope/hercules_kalliope.obj";
    const std::string TEXTURE_PATH = "testmodels/hercules_kalliope/T_Herkules_Kalliope.png";
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    // Define the number of objects to render
    constexpr int MAX_OBJECTS = 3;

    const std::vector validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

    // Define a structure to hold per-object data
    struct GameObject
    {
        // Transform properties
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};

        // Uniform buffer for this object (one per frame in flight)
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VmaAllocation> uniformBuffersAllocation;
        std::vector<void*> uniformBuffersMapped;

        // Descriptor sets for this object (one per frame in flight)
        std::vector<vk::raii::DescriptorSet> descriptorSets;

        // Calculate model matrix based on position, rotation, and scale
        glm::mat4 getModelMatrix() const
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, scale);
            return model;
        }
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
        }

        static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            return {
                vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
                vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
                vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
            };
        }

        bool operator==(const Vertex& other) const
        {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }
    };


    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };


    class HelloTriangleApplication
    {
    public:
        void run();

    private:
        GLFWwindow* window = nullptr;
        GLFWmonitor** monitors = nullptr;
        int monitors_count = 0;
        vk::raii::Context context;
        vk::raii::Instance instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
        vk::raii::SurfaceKHR surface = nullptr;
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
        vk::raii::Device device = nullptr;
        uint32_t queueIndex = ~0;
        vk::raii::Queue queue = nullptr;

        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> swapChainImages;
        vk::Format swapChainImageFormat = vk::Format::eUndefined;
        vk::Extent2D swapChainExtent;
        std::vector<vk::raii::ImageView> swapChainImageViews;

        vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
        vk::raii::PipelineLayout pipelineLayout = nullptr;
        vk::raii::Pipeline graphicsPipeline = nullptr;

        VkImage colorImage = nullptr;
        VmaAllocation colorImageAllocation = nullptr;
        vk::raii::ImageView colorImageView = nullptr;

        VkImage depthImage = nullptr;
        VmaAllocation depthImageAllocation = nullptr;
        vk::raii::ImageView depthImageView = nullptr;

        uint32_t mipLevels = 0;
        VkImage textureImage = nullptr;
        VmaAllocation textureImageAllocation = nullptr;
        vk::raii::ImageView textureImageView = nullptr;
        vk::raii::Sampler textureSampler = nullptr;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer = {};
        VmaAllocation vertexBufferAllocation = {};
        VkBuffer indexBuffer = nullptr;
        VmaAllocation indexBufferAllocation = nullptr;

        //std::vector<VkBuffer> uniformBuffers;
        //std::vector<VmaAllocation> uniformBuffersAllocation;
        //std::vector<void*> uniformBuffersMapped;

        vk::raii::DescriptorPool descriptorPool = nullptr;
        //std::vector<vk::raii::DescriptorSet> descriptorSets;

        vk::raii::CommandPool commandPool = nullptr;
        std::vector<vk::raii::CommandBuffer> commandBuffers;

        std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
        std::vector<vk::raii::Fence> inFlightFences;
        uint32_t semaphoreIndex = 0;
        uint32_t currentFrame = 0;

        // Array of game objects to render
        std::array<GameObject, MAX_OBJECTS> gameObjects;

        bool framebufferResized = false;

        VmaAllocator allocator;

        Camera camera_;
        double delta_time = 0.0;
        double last_frame_time = 0.0;

        //ImGui
        ImGuiIO* io = nullptr;
        vk::raii::DescriptorPool imgui_descriptor_pool = nullptr;
        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        std::vector<const char*> requiredDeviceExtension = {
            vk::KHRSwapchainExtensionName,
            vk::KHRSpirv14ExtensionName,
            vk::KHRSynchronization2ExtensionName,
            vk::KHRCreateRenderpass2ExtensionName,
            vk::KHRDynamicRenderingExtensionName
        };

        void initWindow();
        void initVulkan();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void mainLoop();
        void cleanupSwapChain();
        void cleanup();
        void recreateSwapChain();
        void createInstance();
        void createSurface();
        void createSwapChain();
        void createImageViews();
        void createGraphicsPipeline();
        void createCommandPool();
        void createTextureImage();
        void generateMipmaps(VkImage& image, vk::Format imageFormat, int32_t texWidth,
                             int32_t texHeight, uint32_t mipLevels);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                         vk::SampleCountFlagBits numSamples, vk::Format format,
                         vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                         VkImage& image, VmaAllocation& imageAllocation);
        void transitionImageLayout(const VkImage& image, const vk::ImageLayout oldLayout,
                                   const vk::ImageLayout newLayout, uint32_t mipLevels);
        void copyBufferToImage(const VkBuffer& buffer, VkImage& image,
                               uint32_t width, uint32_t height);
        std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands();
        void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer);
        void createCommandBuffers();
        void recordCommandBuffer(uint32_t imageIndex);
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                          vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer,
                        VkBuffer dstBuffer,
                        VkDeviceSize size);
        void createIndexBuffer();
        void createVertexBuffer();
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        void transition_image_layout(
            uint32_t imageIndex,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::AccessFlags2 src_access_mask,
            vk::AccessFlags2 dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask
        );
        void createSyncObjects();
        void drawFrame();
        void createDescriptorSetLayout();
        void createUniformBuffers();
        void setupDebugMessenger();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateUniformBuffer(uint32_t currentImage);
        void createTextureImageView();
        void createTextureSampler();
        void createDepthResources();
        void loadModel();
        vk::SampleCountFlagBits getMaxUsableSampleCount();
        void createColorResources();
        void transition_image_layout_custom(
            VkImage& image,
            vk::ImageLayout old_layout,
            vk::ImageLayout new_layout,
            vk::AccessFlags2 src_access_mask,
            vk::AccessFlags2 dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask,
            vk::ImageAspectFlags aspect_mask
        );
        void setupGameObjects();
        void createAllocator(VkInstance instance, VkPhysicalDevice physicalDevice,
                             VkDevice device);
        void vmaCleanup();
        void initImGui();
        void paintImGui();
        void initCamera();

        vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                                       vk::FormatFeatureFlags features);
        vk::Format findDepthFormat();
        bool hasStencilComponent(vk::Format format);

        std::vector<const char*> getRequiredExtensions();
        vk::raii::ImageView createImageView(const VkImage& image, vk::Format format,
                                            vk::ImageAspectFlags aspectFlags,
                                            uint32_t mipLevels) const;

        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                              vk::DebugUtilsMessageTypeFlagsEXT type,
                                                              const vk::DebugUtilsMessengerCallbackDataEXT*
                                                              pCallbackData,
                                                              void*)
        {
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << "\n";
            return vk::False;
        }

        static vk::Format chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        {
            const auto formatIt = std::ranges::find_if(availableFormats,
                                                       [](const auto& format)
                                                       {
                                                           return format.format == vk::Format::eB8G8R8A8Srgb &&
                                                               format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                                                       });
            return formatIt != availableFormats.end() ? formatIt->format : availableFormats[0].format;
        }

        static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
        {
            return std::ranges::any_of(availablePresentModes,
                                       [](const vk::PresentModeKHR value)
                                       {
                                           return vk::PresentModeKHR::eMailbox == value;
                                       })
                       ? vk::PresentModeKHR::eMailbox
                       : vk::PresentModeKHR::eFifo;
        }

        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                return capabilities.currentExtent;
            }
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            return {
                std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
        }

        static std::vector<char> readFile(const std::string& filename)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                throw std::runtime_error("failed to open file!");
            }
            std::vector<char> buffer(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            file.close();
            return buffer;
        }

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
        {
            auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }

        [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const
        {
            vk::ShaderModuleCreateInfo createInfo{};
            createInfo.codeSize = code.size() * sizeof(char);
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            vk::raii::ShaderModule shaderModule{device, createInfo};

            return shaderModule;
        }
    };
}
