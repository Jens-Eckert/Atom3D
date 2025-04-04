#ifndef MESH_HPP
#define MESH_HPP

#include "AllocatedBuffer.hpp"
#include "Vertex.hpp"

struct MvpUbo {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct Mesh {
    std::vector<Vertex> vertices;

    static void triangle(Mesh& mesh) {
        mesh.vertices.resize(3);

        mesh.vertices[0].pos = {1.f, 1.f, 0.0f};
        mesh.vertices[1].pos = {-1.f, 1.f, 0.0f};
        mesh.vertices[2].pos = {0.f, -1.f, 0.0f};

        // vertex colors, all green
        mesh.vertices[0].color = {1.f, 0.f, 0.0f};
        mesh.vertices[1].color = {0.f, 1.f, 0.0f};
        mesh.vertices[2].color = {0.f, 0.f, 1.0f};
    }
};

#endif