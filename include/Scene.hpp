#ifndef SCENE_HPP
#define SCENE_HPP

#include "Mesh.hpp"

struct Scene {
    Scene() = default;

    virtual void uploadMeshes(vma::Allocator&, vk::CommandBuffer, vk::Queue) = 0;

    std::vector<Mesh> meshes;
    AllocatedBuffer buffer;  // Both vertex and Index buffers can be housed here (I think)
};

struct MainScene : public Scene {
    MainScene();

    void uploadMeshes(vma::Allocator&, vk::CommandBuffer, vk::Queue) override;
};

#endif