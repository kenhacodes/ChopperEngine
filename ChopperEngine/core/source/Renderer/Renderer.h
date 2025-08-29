#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include <Renderer/Device.h>
#include <Renderer/Pipeline.h>
#include <Renderer/Swapchain.h>
#include <Renderer/Window.h>
#include <Renderer/Resources.h>
#include <Renderer/Image.h>
#include <vma/vk_mem_alloc.h>

#include "Core/Camera.h"
#include "Core/Mesh.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class Window;
class Device;
//class Mesh;

class Renderer
{
public:
    void init();
    void mainLoop(double dt);
    void cleanUp();

    // TODO: This is hardcoded change in the future
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(uint32_t imageIndex);

    Renderer() = default;

    vk::raii::Context context_;
    vk::raii::Instance instance_ = nullptr;
    vk::raii::DebugUtilsMessengerEXT debug_messenger_ = nullptr;
    vk::raii::SurfaceKHR surface_ = nullptr;

    Device device_;

    VmaAllocator allocator_ = VK_NULL_HANDLE;

    Swapchain swapchain_;

    Pipeline pipeline_;
    vk::raii::DescriptorPool descriptor_pool = nullptr;
    vk::raii::DescriptorPool imgui_descriptor_pool = nullptr;
    vk::raii::Sampler sampler_model_texture = nullptr;

    // resources
    TextureImage model_texture;
    Image color_image;
    Image depth_image;

    // Mesh game objects
    Mesh mesh;
    std::array<GameObject, 3> gameObjects;

    // command 
    vk::raii::CommandPool command_pool = nullptr;
    std::vector<vk::raii::CommandBuffer> command_buffers_;

    // sync objects 
    std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
    std::vector<vk::raii::Fence> inFlightFences;

    Window window_;
    Camera camera_;
    

    vk::SampleCountFlagBits msaa_samples_ = vk::SampleCountFlagBits::e1;
    uint32_t queue_index_ = ~0;
    vk::raii::Queue queue = nullptr;
    bool framebuffer_resized_ = false;
    uint32_t semaphoreIndex = 0;
    uint32_t currentFrame = 0;

    ImGuiIO* imgui_io = nullptr;
    
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

private:
    void drawFrame();
    void createAllocator();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT*
                                                          pCallbackData,
                                                          void*)
    {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << "\n";
        return vk::False;
    }
};
