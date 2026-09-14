#pragma once

#include <core/factory.h>

#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

class AssetId
{
  public:
    AssetId(const AssetId &) = default;
    AssetId &operator=(const AssetId &) = default;

    AssetId(AssetId &&) = default;
    AssetId &operator=(AssetId &&) = default;

    static AssetId Create()
    {
        static std::mutex mutex;
        static std::uint64_t next = 1;
        std::lock_guard lock(mutex);
        return {next++};
    }

    static AssetId Invalid()
    {
        return {0};
    }

    explicit operator bool() const
    {
        return _value != 0;
    }

    auto operator<=>(const AssetId &) const = default;

    std::uint64_t get() const
    {
        return _value;
    }

  private:
    AssetId(std::uint64_t value) : _value(value)
    {
    }
    std::uint64_t _value;
};

struct Camera
{
    AssetId id;
    std::string name;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float fov = glm::radians(60.0f);
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
};

struct Light
{
    AssetId id;
    std::string name;
    glm::vec3 position;
    glm::vec4 colorAmbient;
    glm::vec4 colorDiffuse;
    glm::vec4 colorSpecular;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::uvec4 skinIndices;
    glm::vec4 skinWeights;
};

struct ModelTransform
{
    std::string name;
    int32_t parent = -1;
    glm::mat4 localTransform;
};

struct ModelPart
{
    std::string name;
    uint32_t transformIndex = 0;

    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

struct Mesh
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct Transform
{
    std::string name;
    glm::mat4 transform;
};

struct Model
{
    AssetId id;
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ModelTransform> transforms;
    std::vector<ModelPart> parts;
};

struct Texture
{
    std::string name;
    int width;
    int height;
    std::vector<glm::vec4> pixels;
};

struct Material
{
    AssetId id;
    std::string name;
    std::vector<std::string> fragmentShaders;
    std::vector<std::string> vertexShaders;
    std::vector<Texture> textures;
};

class AssetContainer : public Managed<AssetContainer>, public std::enable_shared_from_this<AssetContainer>
{
  public:
    std::shared_ptr<const Material> GetMaterial(std::string_view name) const
    {
        auto it = std::find_if(_materials.begin(), _materials.end(),
                               [name](const auto &material) { return material.name == name; });
        if (it == _materials.end())
            return nullptr;

        return {shared_from_this(), &*it};
    }

    std::shared_ptr<const Camera> GetCamera(std::string_view name) const
    {
        auto it =
            std::find_if(_cameras.begin(), _cameras.end(), [name](const auto &camera) { return camera.name == name; });
        if (it == _cameras.end())
            return nullptr;

        return {shared_from_this(), &*it};
    }

    std::shared_ptr<const Model> GetModel(std::string_view name) const
    {
        auto it =
            std::find_if(_models.begin(), _models.end(), [name](const auto &model) { return model.name == name; });
        if (it == _models.end())
            return nullptr;

        return {shared_from_this(), &*it};
    }

    AssetContainer(Factory, std::string name, std::vector<Camera> cameras, std::vector<Light> lights,
                   std::vector<Model> models, std::vector<Material> materials)
        : _name(std::move(name)), _cameras(std::move(cameras)), _lights(std::move(lights)), _models(std::move(models)),
          _materials(std::move(materials))
    {
    }

  private:
    const std::string _name;
    const std::vector<Camera> _cameras;
    const std::vector<Light> _lights;
    const std::vector<Model> _models;
    const std::vector<Material> _materials;
};

using Asset = std::variant<Camera, Light, Model>;

class AssetError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct AssetOptions
{
};

class AssetManager
{
  public:
    AssetManager(AssetOptions options = {});
    ~AssetManager() noexcept;
    AssetContainer::Ptr LoadAssets(std::filesystem::path p);

  private:
    AssetOptions _options;
    std::map<std::string, std::weak_ptr<AssetContainer>> _assets;
};