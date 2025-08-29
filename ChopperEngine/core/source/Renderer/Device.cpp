#include "Device.h"

void Device::init(vk::raii::Instance& instance, vk::raii::SurfaceKHR& surface, uint32_t& queue_index,
              vk::raii::Queue& queue)
{
    pickPhysicalDevice(instance);
    createLogicalDevice(surface, queue_index, queue);
}

vk::SampleCountFlagBits Device::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties physicalDeviceProperties = physical_device_.getProperties();

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

void Device::pickPhysicalDevice(vk::raii::Instance& instance)
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

            bool supportsAllRequiredExtensions = std::ranges::all_of(required_device_extension_,
                                                                     [&availableDeviceExtensions](
                                                                     auto const& required_device_extension_)
                                                                     {
                                                                         return std::ranges::any_of(
                                                                             availableDeviceExtensions,
                                                                             [required_device_extension_](
                                                                             auto const& availableDeviceExtension)
                                                                             {
                                                                                 return strcmp(
                                                                                     availableDeviceExtension.
                                                                                     extensionName,
                                                                                     required_device_extension_) == 0;
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

    if (devIter != devices.end()) physical_device_ = *devIter;
    else throw std::runtime_error("failed to find a suitable GPU!");
}

void Device::createLogicalDevice(vk::raii::SurfaceKHR& surface, uint32_t& queue_index,
                             vk::raii::Queue& queue)
{
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physical_device_.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physical_device_.getSurfaceSupportKHR(qfpIndex, *surface))
        {
            // found a queue family that supports both graphics and present
            queue_index = qfpIndex;
            break;
        }
    }
    if (queue_index == ~0)
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
    deviceQueueCreateInfo.queueFamilyIndex = queue_index;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

    vk::DeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = vk::DeviceCreateInfo::structureType;
    deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(required_device_extension_.size());
    deviceCreateInfo.ppEnabledExtensionNames = required_device_extension_.data();

    device_ = vk::raii::Device(physical_device_, deviceCreateInfo);

    queue = vk::raii::Queue(device_, queue_index, 0);
}
