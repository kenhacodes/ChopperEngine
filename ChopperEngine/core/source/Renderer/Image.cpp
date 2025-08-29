#include "Image.h"
#include "Renderer/Renderer.h"
// ------------------------------
// Image class
// ------------------------------

Image::Image()
{
}

Image::Image(VkImage& image, VmaAllocation& image_allocation)
{
    image_ = image;
    image_allocation_ = image_allocation;
}

vk::raii::ImageView Image::createImageView(const VkImage& image, vk::Format format,
                                           vk::ImageAspectFlags aspectFlags,
                                           uint32_t mip_levels, Renderer& renderer) const
{
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange = {aspectFlags, 0, mip_levels, 0, 1};
    return vk::raii::ImageView(renderer.device_.device_, viewInfo);
}

void Image::createImage(uint32_t width, uint32_t height, uint32_t mip_levels,
                        vk::SampleCountFlagBits num_samples, vk::Format format,
                        vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                        Image& image, Renderer& renderer)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mip_levels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = static_cast<VkFormat>(format);
    imageInfo.tiling = static_cast<VkImageTiling>(tiling);
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(num_samples);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // VRAM preferred
    // For GPU-only images (textures, depth, color attachments), device-local is usually best.

    if (vmaCreateImage(renderer.allocator_, &imageInfo, &allocInfo, &image.image_, &image.image_allocation_,
                       nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image with VMA!");
    }
}

void Image::customTransitionImageLayout(VkImage& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                                        vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
                                        vk::PipelineStageFlags2 src_stage_mask,
                                        vk::PipelineStageFlags2 dst_stage_mask, vk::ImageAspectFlags aspect_mask, Renderer& renderer)
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
    
    renderer.command_buffers_[renderer.currentFrame].pipelineBarrier2(dependency_info);
}

// ------------------------------
// Texture Image class
// ------------------------------

TextureImage::TextureImage()
{
}

void TextureImage::createTextureSampler(vk::raii::Sampler& textureSampler, Renderer& renderer)
{
    vk::PhysicalDeviceProperties properties = renderer.device_.physical_device_.getProperties();
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
    textureSampler = vk::raii::Sampler(renderer.device_.device_, samplerInfo);
}


void TextureImage::generateMipmaps(TextureImage& image, vk::Format image_format, int32_t tex_width, int32_t tex_height,
                                   Renderer& renderer)
{
    // Check if image format supports linear blit-ing
    vk::FormatProperties formatProperties = renderer.device_.physical_device_.getFormatProperties(image_format);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = ChopperBuffer::beginSingleTimeCommands(renderer);

    vk::ImageMemoryBarrier barrier = {};
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image.image_base_.image_;

    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = tex_width;
    int32_t mipHeight = tex_height;

    for (uint32_t i = 1; i < image.mip_levels_; i++)
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

        commandBuffer->blitImage(image.image_base_.image_, vk::ImageLayout::eTransferSrcOptimal,
                                 image.image_base_.image_,
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

    barrier.subresourceRange.baseMipLevel = image.mip_levels_ - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                   {}, {}, {}, barrier);

    ChopperBuffer::endSingleTimeCommands(*commandBuffer, renderer);
}

void TextureImage::transitionImageLayout(const TextureImage& texture_image, const vk::ImageLayout old_layout,
                                         const vk::ImageLayout new_layout, Renderer& renderer)
{
    auto commandBuffer = ChopperBuffer::beginSingleTimeCommands(renderer);

    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.image = texture_image.image_base_.image_;
    barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, texture_image.mip_levels_, 0, 1};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout ==
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
    ChopperBuffer::endSingleTimeCommands(*commandBuffer, renderer);
}
