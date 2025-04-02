#include "Scene.hpp"

#include <iostream>

MainScene::MainScene() {
    Mesh triangle;

    Mesh::triangle(triangle);

    meshes.push_back(triangle);
}

// TODO: Make this more modular
/// @brief Creates a Buffer for scene meshes, stores mesh vertices/indices in it.
/// @param allocator
/// @param commandBuffer
/// @param subQueue
void MainScene::uploadMeshes(vma::Allocator& allocator, vk::CommandBuffer commandBuffer, vk::Queue subQueue) {
    float vertexBufferSize = sizeof(Vertex) * meshes[0].vertices.size();

    // Create Staging Buffer
    vk::BufferUsageFlags stgbufUsage = vk::BufferUsageFlagBits::eTransferSrc;

    auto [stagingBuffer, stgAlloc] = AllocatedBuffer::createBuffer(allocator, vertexBufferSize, stgbufUsage, vma::MemoryUsage::eAuto);

    void* mem = allocator.mapMemory(stgAlloc);
    memcpy(mem, meshes[0].vertices.data(), vertexBufferSize);
    allocator.unmapMemory(stgAlloc);

    // Create Vertex/IndexBuffer (No index for now)
    vk::BufferUsageFlags vbufUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

    buffer = AllocatedBuffer::createBuffer(allocator, vertexBufferSize, vbufUsage, vma::MemoryUsage::eAuto);

    // Copy buffers

    AllocatedBuffer::copyBuffer(stagingBuffer, buffer.buffer, vertexBufferSize, commandBuffer, subQueue);

    // Destroy Staging Buffer
    allocator.destroyBuffer(stagingBuffer, stgAlloc);
}