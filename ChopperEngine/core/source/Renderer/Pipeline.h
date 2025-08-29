#pragma once
#include <vulkan/vulkan_raii.hpp>


#include "Core/common.h"

class Renderer;

class Pipeline
{
public:
    Pipeline() = default;
    
    void init(Renderer& renderer_ref);

    void createDescriptorSetLayout(); // Pipeline
    void createGraphicsPipeline(); // Pipeline

    Renderer* renderer_ref_ = nullptr;
    vk::raii::DescriptorSetLayout descriptor_set_layout_ = nullptr;
    vk::raii::PipelineLayout pipeline_layout_ = nullptr;
    vk::raii::Pipeline graphics_pipeline_ = nullptr;

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    vk::Format findDepthFormat();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                        vk::ImageTiling tiling, vk::FormatFeatureFlags features);
};
