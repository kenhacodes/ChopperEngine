#pragma once
#include <vulkan/vulkan_core.h>
#include <memory>
#include "Core/common.h"
#include "Renderer/Renderer.h"
#include "Renderer/Image.h"
#include "Core/Mesh.h"

namespace ChopperBuffer
{
    void copyBufferToImage(const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height, Renderer& renderer);

    std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands(Renderer& renderer);

    void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, Renderer& renderer);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                    Renderer& renderer);

    void cleanImage(Image& image, Renderer& renderer);
    void cleanTextureImage(TextureImage& image, Renderer& renderer);
    void cleanMesh(Mesh& mesh, Renderer& renderer);
    void cleanGameObject(GameObject& game_object, Renderer& renderer);
}
