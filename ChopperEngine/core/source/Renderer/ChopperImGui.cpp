#include "ChopperImGui.h"

void ChopperImGui::initImgui(vk::raii::DescriptorPool& imgui_descriptor_pool, Renderer& renderer)
{
    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
    std::array pool_size{
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
        vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000),
    };

    Descriptors::createDescriptorPool(imgui_descriptor_pool, pool_size, 1000, renderer);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(renderer.window_.window_, &w, &h);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    renderer.imgui_io = &ImGui::GetIO();
    //(void)io;
    renderer.imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    renderer.imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    ImGui::GetStyle().FontScaleMain = 1.3f;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(renderer.window_.window_, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = vk::ApiVersion14;
    // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = *renderer.instance_;
    init_info.PhysicalDevice = *renderer.device_.physical_device_;
    init_info.Device = *renderer.device_.device_;
    init_info.QueueFamily = renderer.queue_index_;
    init_info.Queue = *renderer.queue;
    init_info.DescriptorPool = *imgui_descriptor_pool;
    init_info.DescriptorPoolSize = 0;
    init_info.RenderPass = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VkSampleCountFlagBits(renderer.msaa_samples_);
    init_info.UseDynamicRendering = true;
    init_info.Subpass = 0;

    init_info.Allocator = nullptr;
    //init_info.CheckVkResultFn = check_vk_result;

    init_info.PipelineRenderingCreateInfo = {};
    init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineRenderingCreateInfo.pNext = nullptr;
    init_info.PipelineRenderingCreateInfo.viewMask = 0;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    VkFormat format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    init_info.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    ImGui_ImplVulkan_Init(&init_info);

    // Load Fonts
    //style.FontSizeBase = 20.0f;
    ImFont* font = renderer.imgui_io->Fonts->AddFontFromFileTTF("../core/resources/fonts/NunitoSans.ttf");
    IM_ASSERT(font != nullptr);
    //io->Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    //io->Fonts->AddFontDefault();
    //io->Fonts->Build();
    //ImGui::GetStyle().FontSizeBase = 20.0f;
}

void ChopperImGui::imgui_initFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ChopperImGui::imgui_endFrame()
{
    ImGui::Render();
}
