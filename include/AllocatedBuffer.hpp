#ifndef ALLOCATED_BUFFER_HPP
#define ALLOCATED_BUFFER_HPP

#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

struct AllocatedBuffer {
    vk::Buffer buffer;
    vma::Allocation allocation;

    static AllocatedBuffer createBuffer(vma::Allocator& allocator, size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memUsage) {
        vk::BufferCreateInfo buffInfo;
        buffInfo.setSize(size)
            .setUsage(usage);

        vma::AllocationCreateInfo vmaAllocInfo;
        vmaAllocInfo.setUsage(memUsage)
            .setFlags(vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite);

        AllocatedBuffer newBuff;

        auto [b, a] = allocator.createBuffer(buffInfo, vmaAllocInfo);

        newBuff.buffer = b;
        newBuff.allocation = a;

        return newBuff;
    }

    static void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::CommandBuffer commandBuffer, vk::Queue subQueue) {
        commandBuffer.reset();

        vk::CommandBufferBeginInfo info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

        // Record
        commandBuffer.begin(info);

        vk::BufferCopy region;
        region.setSize(size);
        commandBuffer.copyBuffer(src, dst, region);

        commandBuffer.end();

        // Submit
        vk::SubmitInfo subInfo;
        subInfo.setCommandBuffers(commandBuffer);

        subQueue.submit(subInfo, nullptr);
        subQueue.waitIdle();
    }
};

#endif