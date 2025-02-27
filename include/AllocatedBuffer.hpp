#ifndef ALLOCATED_BUFFER_HPP
#define ALLOCATED_BUFFER_HPP

#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

struct AllocatedBuffer {
    vk::Buffer buffer;
    vma::Allocation allocation;
};

#endif