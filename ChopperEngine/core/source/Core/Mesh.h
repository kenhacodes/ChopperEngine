#pragma once
#include <cstdint>
#include <tinyobjloader/tiny_obj_loader.h>
#include "common.h"
class Renderer;

class Mesh
{
public:
    
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    VkBuffer vertex_buffer_ = {};
    VmaAllocation vertex_buffer_allocation_ = {};
    VkBuffer index_buffer_ = nullptr;
    VmaAllocation index_buffer_allocation_ = nullptr;
    
    void loadModel(std::string path, Mesh& model);
    void createVertexBuffer(Mesh& mesh, Renderer& renderer);
    void createIndexBuffer(Mesh& mesh, Renderer& renderer);
    
};
