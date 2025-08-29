#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <stb/stb_image.h>

class Renderer;

class Image
{
public:
    Image(VkImage& image, VmaAllocation& image_allocation);
    Image();
   

    VkImage image_ = nullptr;
    VmaAllocation image_allocation_ = nullptr;
    vk::raii::ImageView image_view_ = nullptr;

    vk::raii::ImageView createImageView(const VkImage& image, vk::Format format,
                                        vk::ImageAspectFlags aspectFlags,
                                        uint32_t mip_levels, Renderer& renderer) const;

    // Maybe i need to add the option for how its stored (VRAM or RAM) for advanced use.
    void createImage(uint32_t width, uint32_t height, uint32_t mip_levels,
                     vk::SampleCountFlagBits num_samples, vk::Format format,
                     vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                     Image& image, Renderer& renderer);

    void customTransitionImageLayout(
        VkImage& image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags aspect_mask,
        Renderer& renderer
    );
};

class TextureImage
{
public:
    TextureImage();
    ~TextureImage() = default;

    Image image_base_;
    uint32_t mip_levels_ = 0;
    uint32_t texture_id = -1;

    // -> Sampler, TODO: Add optional sampler settings
    void createTextureSampler(vk::raii::Sampler& textureSampler, Renderer& renderer);

    void generateMipmaps(TextureImage& image, vk::Format image_format, int32_t tex_width,
                         int32_t tex_height, Renderer& renderer);

    // For textures
    void transitionImageLayout(const TextureImage& texture_image, const vk::ImageLayout old_layout,
                               const vk::ImageLayout new_layout, Renderer& renderer);
};
