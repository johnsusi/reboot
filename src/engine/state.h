#pragma once

#include "asset.h"

#include <optional>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

struct TransformComponent
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct MeshRenderComponent
{
    bool visible = false;
    std::shared_ptr<const Model> model;
};

struct CameraComponent
{
    std::shared_ptr<const Camera> camera;

    glm::mat4 getProjectionMatrix() const
    {
        return glm::perspective(camera->fov, camera->aspectRatio, camera->nearClip, camera->farClip);
    }

    glm::mat4 getViewMatrix(const TransformComponent &transform) const
    {
        glm::vec3 position = transform.position;
        glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
        glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);
        return glm::lookAt(position, position + forward, up);
    }
};

struct Entity
{
    std::optional<TransformComponent> transform;
    std::optional<CameraComponent> camera;
    std::optional<MeshRenderComponent> meshRenderer;
};

struct GameState
{
    bool running = true;
    std::shared_ptr<const Camera> camera;
    std::shared_ptr<const Material> material;
    std::vector<Entity> entities;
};