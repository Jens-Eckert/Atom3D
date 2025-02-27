#ifndef VERTEX_HPP
#define VERTEX_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttrDesc() {
        vk::VertexInputAttributeDescription p(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos));
        vk::VertexInputAttributeDescription c(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
        vk::VertexInputAttributeDescription t(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord));

        return {p, c, t};
    }
};

#endif