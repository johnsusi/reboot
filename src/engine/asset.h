#pragma once

#include <filesystem>
#include <variant>
#include <vector>
#include <string>
#include <map>

#include <glm/glm.hpp>

struct Camera
{
    std::string name;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float fov;
    float nearClip;
    float farClip;
};

struct Light
{
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
    std::string name;
    std::vector<Mesh> meshes;
    std::vector<glm::mat4> transforms;
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
    std::string name;
    std::vector<std::string> fragmentShaders;
    std::vector<std::string> vertexShaders;
    std::vector<Texture> textures;
};

struct AssetContainer
{
    std::string name;
    std::vector<Camera> cameras;
    std::vector<Light> lights;
    std::vector<Mesh> meshes;
    std::vector<Transform> transforms;
    std::vector<Material> materials;
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
    void LoadAssets(std::filesystem::path p);

private:
    AssetOptions _options;
    std::map<std::string, AssetContainer> _assets;
};