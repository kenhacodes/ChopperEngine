#include "Renderer.h"

void Resources::createColorResources(Image& image, Renderer& renderer)
{
    vk::Format colorFormat = renderer.swapchain_.swapchain_image_format_;

    image.createImage(
        renderer.swapchain_.swapchain_extent_.width,
        renderer.swapchain_.swapchain_extent_.height,
        1,
        renderer.msaa_samples_,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        image, renderer
    );
    image.image_view_ = image.createImageView(image.image_, colorFormat, vk::ImageAspectFlagBits::eColor, 1, renderer);
}

void Resources::createDepthResources(Image& image, Renderer& renderer)
{
    vk::Format depthFormat = renderer.pipeline_.findDepthFormat();

    image.createImage(
        renderer.swapchain_.swapchain_extent_.width,
        renderer.swapchain_.swapchain_extent_.height,
        1,
        renderer.msaa_samples_,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        image, renderer
    );
    image.image_view_ = image.createImageView(image.image_, depthFormat, vk::ImageAspectFlagBits::eDepth, 1, renderer);

    //transitionImageLayout(depthImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
}

void Resources::createTextureImage(TextureImage& texture_image, std::string path, Renderer& renderer)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    texture_image.mip_levels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");
    
    // 1. Create staging buffer (CPU visible)
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

    if (vmaCreateBuffer(renderer.allocator_, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation,
                        &stagingAllocInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create staging buffer for texture!");
    }

    // Copy pixels into mapped memory
    memcpy(stagingAllocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

    stbi_image_free(pixels);
    
    // 2. Create GPU-only texture image with mip levels

    texture_image.image_base_.createImage(
        texWidth,
        texHeight,
        texture_image.mip_levels_,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | // needed for mipmap generation
        vk::ImageUsageFlagBits::eTransferDst | // we copy into it from staging
        vk::ImageUsageFlagBits::eSampled,
        texture_image.image_base_, renderer
    );
    
    // 3. Transition, copy, generate mipmaps

    texture_image.transitionImageLayout(texture_image, vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eTransferDstOptimal, renderer);

    //transitionImageLayout(texture_image.image_base_.image_, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
    //                      texture_image.mip_levels_);

    ChopperBuffer::copyBufferToImage(stagingBuffer, texture_image.image_base_.image_, static_cast<uint32_t>(texWidth),
                                     static_cast<uint32_t>(texHeight), renderer);

    texture_image.generateMipmaps(texture_image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, renderer);
    
    vmaDestroyBuffer(renderer.allocator_, stagingBuffer, stagingAllocation);
}

void Resources::createTextureImageView(TextureImage& texture_image, Renderer& renderer)
{
    // TODO : vkFormat is hardcoded. I don't know how to get the correct value and might not work on other gpus because of this :/ 
    texture_image.image_base_.image_view_ = texture_image.image_base_.createImageView(
        texture_image.image_base_.image_, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor,
        texture_image.mip_levels_, renderer);
}


void Resources::createUniformBuffers(GameObject& game_object, VmaAllocator& allocator)
{
    game_object.uniformBuffers.clear();
    game_object.uniformBuffersAllocation.clear();
    game_object.uniformBuffersMapped.clear();

    VkDeviceSize bufferSize = sizeof(UBO_MVP);

    game_object.uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    game_object.uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
    game_object.uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

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
            VMA_ALLOCATION_CREATE_MAPPED_BIT; // keep it mapped

        VmaAllocationInfo vmaAllocDetails{};
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
                            &game_object.uniformBuffers[i], &game_object.uniformBuffersAllocation[i],
                            &vmaAllocDetails) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create uniform buffer with VMA!");
        }

        // Already mapped because of the flag
        game_object.uniformBuffersMapped[i] = vmaAllocDetails.pMappedData;
    }
}
