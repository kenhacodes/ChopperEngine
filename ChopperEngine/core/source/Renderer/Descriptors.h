#pragma once
#include <array>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>
#include "Core/common.h"

#include "Renderer.h"
#include "vma/vk_mem_alloc.h"

namespace Descriptors
{
    template <size_t N>
    void createDescriptorPool(vk::raii::DescriptorPool& descriptor_pool,
                              const std::array<vk::DescriptorPoolSize, N>& pool_sizes,
                              uint32_t descriptor_count,
                              Renderer& renderer)
    {
        {
            vk::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
            poolInfo.maxSets = descriptor_count;
            poolInfo.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
            poolInfo.pPoolSizes = pool_sizes.data();

            descriptor_pool = vk::raii::DescriptorPool(renderer.device_.device_, poolInfo);
        }
    }


    void createDescriptorSets(GameObject& game_object, vk::raii::DescriptorPool& descriptor_pool, Image& image,
                              vk::raii::Sampler& sampler, Renderer& renderer);
};
