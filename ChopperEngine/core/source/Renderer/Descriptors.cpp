#include "Descriptors.h"

void Descriptors::createDescriptorSets(GameObject& game_object, vk::raii::DescriptorPool& descriptor_pool, Image& image,
                                       vk::raii::Sampler& sampler, Renderer& renderer)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderer.pipeline_.descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = descriptor_pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    game_object.descriptorSets.clear();
    game_object.descriptorSets = renderer.device_.device_.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = game_object.uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UBO_MVP);

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.sampler = sampler;
        imageInfo.imageView = image.image_view_;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::WriteDescriptorSet descriptor_set0 = {};
        descriptor_set0.dstSet = game_object.descriptorSets[i];
        descriptor_set0.dstBinding = 0;
        descriptor_set0.dstArrayElement = 0;
        descriptor_set0.descriptorCount = 1;
        descriptor_set0.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptor_set0.pBufferInfo = &bufferInfo;


        vk::WriteDescriptorSet descriptor_set1 = {};
        descriptor_set1.dstSet = game_object.descriptorSets[i];
        descriptor_set1.dstBinding = 1;
        descriptor_set1.dstArrayElement = 0;
        descriptor_set1.descriptorCount = 1;
        descriptor_set1.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptor_set1.pImageInfo = &imageInfo;

        std::array descriptorWrites{
            vk::WriteDescriptorSet{
                descriptor_set0
            },
            vk::WriteDescriptorSet{
                descriptor_set1
            }
        };
        renderer.device_.device_.updateDescriptorSets(descriptorWrites, {});
    }
}
