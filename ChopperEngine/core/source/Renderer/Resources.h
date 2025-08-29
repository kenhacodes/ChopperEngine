#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>
class Renderer;
class Image;
class TextureImage;


#include "ChopperBuffer.h"

namespace Resources
{
    void createColorResources(Image& image, Renderer& renderer);

    void createDepthResources(Image& image, Renderer& renderer);

    void createTextureImage(TextureImage& texture_image, std::string path, Renderer& renderer);

    void createTextureImageView(TextureImage& texture_image, Renderer& renderer);

    
    void createUniformBuffers(GameObject& game_object, VmaAllocator& allocator);
    
    
    
};
