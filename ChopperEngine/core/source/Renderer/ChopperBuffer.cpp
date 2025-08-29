#include "ChopperBuffer.h"

void ChopperBuffer::copyBufferToImage(const VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height,
                                      Renderer& renderer)
{
    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = beginSingleTimeCommands(renderer);
    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight    = 0;
    region.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};

    commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
    endSingleTimeCommands(*commandBuffer, renderer);
}

std::unique_ptr<vk::raii::CommandBuffer> ChopperBuffer::beginSingleTimeCommands(Renderer& renderer)
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = renderer.command_pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(
        std::move(vk::raii::CommandBuffers(renderer.device_.device_, allocInfo).front()));

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer->begin(beginInfo);

    return commandBuffer;
}

void ChopperBuffer::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer, Renderer& renderer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*commandBuffer;
    renderer.queue.submit(submitInfo, nullptr);
    renderer.queue.waitIdle();
}

void ChopperBuffer::copyBuffer(VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize size,
                               Renderer& renderer)
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = renderer.command_pool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    vk::raii::CommandBuffer commandCopyBuffer = std::move(
        renderer.device_.device_.allocateCommandBuffers(allocInfo).front());
    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();
    vk::SubmitInfo submit_info = {};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &*commandCopyBuffer;
    renderer.queue.submit(submit_info, nullptr);
    renderer.queue.waitIdle();
}

void ChopperBuffer::cleanImage(Image& image, Renderer& renderer)
{
    vmaDestroyImage(renderer.allocator_, image.image_, image.image_allocation_);
}

void ChopperBuffer::cleanTextureImage(TextureImage& image, Renderer& renderer)
{
    vmaDestroyImage(renderer.allocator_, image.image_base_.image_, image.image_base_.image_allocation_);
}

void ChopperBuffer::cleanMesh(Mesh& mesh, Renderer& renderer)
{
    vmaDestroyBuffer(renderer.allocator_, mesh.vertex_buffer_, mesh.vertex_buffer_allocation_);
    vmaDestroyBuffer(renderer.allocator_, mesh.index_buffer_, mesh.index_buffer_allocation_);
}

void ChopperBuffer::cleanGameObject(GameObject& game_object, Renderer& renderer)
{
    for (int i = 0; i < game_object.uniformBuffers.size(); ++i)
    {
        vmaDestroyBuffer(renderer.allocator_, game_object.uniformBuffers[i], game_object.uniformBuffersAllocation[i]);
    }
}
