#include "Swapchain.h"
#include "Renderer/Renderer.h"   
#include <vma/vk_mem_alloc.h>

#include "ChopperImGui.h"
#include "Window.h"

void Swapchain::init(Renderer& renderer_ref)
{
    renderer_ref_ = &renderer_ref;
    initialized_ = true;
    createSwapChain();
    createImageViews();
}

void Swapchain::createSwapChain()
{
    auto surfaceCapabilities = renderer_ref_->device_.physical_device_.getSurfaceCapabilitiesKHR(
        renderer_ref_->surface_);
    swapchain_image_format_ = chooseSwapSurfaceFormat(
        renderer_ref_->device_.physical_device_.getSurfaceFormatsKHR(renderer_ref_->surface_));
    swapchain_extent_ = chooseSwapExtent(surfaceCapabilities);
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
                        ? surfaceCapabilities.maxImageCount
                        : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.surface = renderer_ref_->surface_;
    swapChainCreateInfo.minImageCount = minImageCount;
    swapChainCreateInfo.imageFormat = swapchain_image_format_;
    swapChainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapChainCreateInfo.imageExtent = swapchain_extent_;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCreateInfo.presentMode = chooseSwapPresentMode(
        renderer_ref_->device_.physical_device_.getSurfacePresentModesKHR(renderer_ref_->surface_));
    swapChainCreateInfo.clipped = true;

    swapchain_ = vk::raii::SwapchainKHR(renderer_ref_->device_.device_, swapChainCreateInfo);
    swapchain_images_ = swapchain_.getImages();
}

void Swapchain::createImageViews()
{
    vk::ImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = swapchain_image_format_;
    imageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    for (auto image : swapchain_images_)
    {
        imageViewCreateInfo.image = image;
        swapchain_image_views_.emplace_back(renderer_ref_->device_.device_, imageViewCreateInfo);
    }
}

void Swapchain::recreateSwapChain(Image& color_image, Image& depth_image)
{
    if (!initialized_)
        throw std::runtime_error("Swapchain not initialized!");

    int width = 0, height = 0;
    glfwGetFramebufferSize(renderer_ref_->window_.window_, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(renderer_ref_->window_.window_, &width, &height);
        glfwWaitEvents();
    }
    
    ChopperImGui::imgui_endFrame();
    renderer_ref_->camera_.setResolution((int)width, (int)height);
    
    renderer_ref_->device_.device_.waitIdle();

    vmaDestroyImage(renderer_ref_->allocator_, color_image.image_, color_image.image_allocation_);
    vmaDestroyImage(renderer_ref_->allocator_, depth_image.image_, depth_image.image_allocation_);

    cleanupSwapChain();
    createSwapChain();
    createImageViews();

    Resources::createColorResources(color_image, *renderer_ref_);
    Resources::createDepthResources(depth_image, *renderer_ref_);
}

void Swapchain::cleanupSwapChain()
{
    swapchain_image_views_.clear();
    swapchain_ = nullptr;
}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(renderer_ref_->window_.window_, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void Swapchain::transition_image_layout(uint32_t imageIndex, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask)
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
        barrier.image = swapchain_images_[imageIndex];
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
    
        renderer_ref_->command_buffers_[renderer_ref_->currentFrame].pipelineBarrier2(dependency_info);
    
}
