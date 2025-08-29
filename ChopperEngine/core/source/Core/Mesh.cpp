#include "Mesh.h"

#include "Renderer/ChopperBuffer.h"


void Mesh::loadModel(std::string path, Mesh& model)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (!uniqueVertices.contains(vertex))
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
                vertices_.push_back(vertex);
            }

            indices_.push_back(uniqueVertices[vertex]);
        }
    }
}

void Mesh::createVertexBuffer(Mesh& mesh, Renderer& renderer)
{
    VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

    // 1. Create staging buffer (CPU-visible, for uploading data)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT; // CPU write access

    VmaAllocationInfo stagingAllocDetails;
    vmaCreateBuffer(renderer.allocator_, &stagingInfo, &stagingAllocInfo,
                    &stagingBuffer, &stagingAllocation, &stagingAllocDetails);

    // Copy vertex data
    memcpy(stagingAllocDetails.pMappedData, vertices_.data(), (size_t)bufferSize);

    // 2. Create GPU-local vertex buffer
    VkBufferCreateInfo vertexInfo = {};
    vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexInfo.size = bufferSize;
    vertexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vertexAllocInfo = {};
    vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // Let VMA choose VRAM

    vmaCreateBuffer(renderer.allocator_, &vertexInfo, &vertexAllocInfo,
                    &vertex_buffer_, &vertex_buffer_allocation_, nullptr);

    // 3. Copy staging → vertex buffer
    ChopperBuffer::copyBuffer(stagingBuffer, vertex_buffer_, bufferSize, renderer);

    // 4. Cleanup staging
    vmaDestroyBuffer(renderer.allocator_, stagingBuffer, stagingAllocation);
}

void Mesh::createIndexBuffer(Mesh& mesh, Renderer& renderer)
{
   // 1. Create staging buffer (CPU-visible, for uploading data)
        vk::DeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;

        VkBufferCreateInfo stagingInfo = {};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = bufferSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.pNext = nullptr;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT; // CPU write access

        VmaAllocationInfo stagingAllocDetails;
        vmaCreateBuffer(renderer.allocator_, &stagingInfo, &stagingAllocInfo,
                        &stagingBuffer, &stagingAllocation, &stagingAllocDetails);

        // Copy vertex data
        memcpy(stagingAllocDetails.pMappedData, indices_.data(), (size_t)bufferSize);

        // 2. Create GPU-local vertex buffer
        VkBufferCreateInfo indexInfo = {};
        indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexInfo.size = bufferSize;
        indexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocInfo = {};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE; // Let VMA choose VRAM

        vmaCreateBuffer(renderer.allocator_, &indexInfo, &indexAllocInfo,
                        &index_buffer_, &index_buffer_allocation_, nullptr);

        // 3. Copy staging → index buffer
   ChopperBuffer::copyBuffer(stagingBuffer, index_buffer_, bufferSize, renderer);

        // 4. Cleanup staging
        vmaDestroyBuffer(renderer.allocator_, stagingBuffer, stagingAllocation);
}
