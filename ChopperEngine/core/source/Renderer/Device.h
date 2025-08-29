#pragma once
#include <vulkan/vulkan_raii.hpp>

class Device
{
public:
    vk::raii::PhysicalDevice physical_device_ = nullptr; // Device
    vk::raii::Device device_ = nullptr;

    Device() = default;
    ~Device() = default;

    void init(vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface, uint32_t& queue_index,
              vk::raii::Queue& queue);

    vk::SampleCountFlagBits getMaxUsableSampleCount();

    std::vector<const char*> required_device_extension_ = {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName,
        vk::KHRDynamicRenderingExtensionName
    };

private:

    void pickPhysicalDevice(vk::raii::Instance& instance); 
    void createLogicalDevice(vk::raii::SurfaceKHR& surface, uint32_t& queue_index,
                             vk::raii::Queue& queue); 
};
