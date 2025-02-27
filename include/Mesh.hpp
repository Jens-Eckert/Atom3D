#ifndef MESH_HPP
#define MESH_HPP

#include "AllocatedBuffer.hpp"
#include "Vertex.hpp"

struct Mesh {
    std::vector<Vertex> vertices;
    AllocatedBuffer vertexBuffer;
};

#endif