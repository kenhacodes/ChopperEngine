#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "Image.h"

class Renderer;


class Swapchain
{
public:

    vk::raii::SwapchainKHR swapchain_ = nullptr;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::raii::ImageView> swapchain_image_views_;
    vk::Extent2D swapchain_extent_;
    vk::Format swapchain_image_format_ = vk::Format::eUndefined;
    Swapchain() = default;

    void init(Renderer& renderer_ref);

    void recreateSwapChain(Image& color_image, Image& depth_image);
    void cleanupSwapChain();

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    );
    
private:

    bool initialized_ = false;
    Renderer* renderer_ref_;
    
    void createSwapChain();
    void createImageViews();
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);


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
};
