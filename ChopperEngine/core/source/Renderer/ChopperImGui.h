#pragma once
#include "Descriptors.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

namespace ChopperImGui
{
    void initImgui(vk::raii::DescriptorPool& imgui_descriptor_pool, Renderer& renderer);
    void imgui_initFrame();
    void imgui_endFrame();
};
