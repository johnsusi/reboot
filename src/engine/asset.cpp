#include "asset.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std::literals;

namespace
{

class VertexBuilder
{
  public:
    VertexBuilder &SetPosition(double x, double y, double z)
    {
        _position = glm::vec3(x, y, z);
        return *this;
    }
    VertexBuilder &SetNormal(double x, double y, double z)
    {
        _normal = glm::vec3(x, y, z);
        return *this;
    }
    VertexBuilder &SetTexCoord(double u, double v)
    {
        _texCoord = glm::vec2(u, v);
        return *this;
    }
    Vertex Build()
    {
        return Vertex{
            .position = _position,
            .normal = _normal,
            .texCoord = _texCoord,
        };
    }

  private:
    glm::vec3 _position;
    glm::vec3 _normal;
    glm::vec2 _texCoord;
};

class ModelBuilder
{
  public:
    ModelBuilder &SetName(std::string name);
    ModelBuilder &AddVertex(Vertex vertex);
    ModelBuilder &AddIndex(uint32_t index);
    ModelBuilder &AddTransform(ModelTransform transform);
    ModelBuilder &AddTransform(auto &&...args)
        requires(sizeof...(args) != 1 || !(std::derived_from<std::decay_t<decltype(args)>, ModelTransform> && ...))
    {
        _transforms.emplace_back(std::forward<decltype(args)>(args)...);
        return *this;
    }
    ModelBuilder &AddPart(ModelPart part);
    Model Build();

  private:
    std::string _name;
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;
    std::vector<ModelTransform> _transforms;
    std::vector<ModelPart> _parts;
};

glm::mat4 ToGlm(const aiMatrix4x4 &m)
{
    glm::mat4 result;

    result[0][0] = m.a1;
    result[1][0] = m.a2;
    result[2][0] = m.a3;
    result[3][0] = m.a4;

    result[0][1] = m.b1;
    result[1][1] = m.b2;
    result[2][1] = m.b3;
    result[3][1] = m.b4;

    result[0][2] = m.c1;
    result[1][2] = m.c2;
    result[2][2] = m.c3;
    result[3][2] = m.c4;

    result[0][3] = m.d1;
    result[1][3] = m.d2;
    result[2][3] = m.d3;
    result[3][3] = m.d4;

    return result;
}

void printNodeHierarchy(const aiNode *node, int depth = 0)
{
    // Indentation for visual hierarchy
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";

    // Print the current node name
    std::cout << node->mName.C_Str() << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mNumMeshes << std::endl;
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        for (int i = 0; i < depth; ++i)
            std::cout << "  ";
        std::cout << "  Mesh index: " << node->mMeshes[i] << std::endl;
    }

    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.a1 << ", " << node->mTransformation.a2 << ", " << node->mTransformation.a3
              << ", " << node->mTransformation.a4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.b1 << ", " << node->mTransformation.b2 << ", " << node->mTransformation.b3
              << ", " << node->mTransformation.b4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.c1 << ", " << node->mTransformation.c2 << ", " << node->mTransformation.c3
              << ", " << node->mTransformation.c4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.d1 << ", " << node->mTransformation.d2 << ", " << node->mTransformation.d3
              << ", " << node->mTransformation.d4 << std::endl;

    // Recurse into children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        printNodeHierarchy(node->mChildren[i], depth + 1);
    }
}

aiNode *findNode(aiNode *node, std::string_view name)
{
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        if (node->mChildren[i]->mName.C_Str() == name)
            return node->mChildren[i];
    }
    return nullptr;
}

void addTransform(const aiNode &node, int32_t parentTransform, ModelBuilder &modelBuilder)
{
    modelBuilder.AddTransform(node.mName.C_Str(), parentTransform, ToGlm(node.mTransformation));
}

void addMesh(aiMesh &mesh, ModelBuilder &modelBuilder)
{
    for (unsigned int i = 0; i < mesh.mNumVertices; ++i)
    {
        VertexBuilder vertexBuilder;
        vertexBuilder.SetPosition(mesh.mVertices[i].x, mesh.mVertices[i].y, mesh.mVertices[i].z);
        if (mesh.mNormals)
            vertexBuilder.SetNormal(mesh.mNormals[i].x, mesh.mNormals[i].y, mesh.mNormals[i].z);
        if (mesh.mTextureCoords[0])
            vertexBuilder.SetTexCoord(mesh.mTextureCoords[0][i].x, mesh.mTextureCoords[0][i].y);
        modelBuilder.AddVertex(vertexBuilder.Build());
    }
    for (unsigned int i = 0; i < mesh.mNumFaces; ++i)
    {
        auto face = mesh.mFaces[i];
        if (face.mNumIndices == 3)
            modelBuilder.AddIndex(face.mIndices[0]).AddIndex(face.mIndices[1]).AddIndex(face.mIndices[2]);
    }
}

void loadModel(const aiScene &scene, const aiNode &node, ModelBuilder &modelBuilder)
{
    addTransform(node, -1, modelBuilder);
    for (unsigned int i = 0; i < node.mNumMeshes; ++i)
    {
        addMesh(*scene.mMeshes[node.mMeshes[i]], modelBuilder);
    }
    for (unsigned int i = 0; i < node.mNumChildren; ++i)
    {
        loadModel(scene, *node.mChildren[i], modelBuilder);
    }
}

void loadModel(std::filesystem::path filePath, std::string rootNode)
{
    auto extension = filePath.extension().string();

    Assimp::Importer importer;
    if (importer.IsExtensionSupported(extension))
    {
        const aiScene *scene = importer.ReadFile(filePath.string(), aiProcess_Triangulate);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            throw AssetError("Failed to load mesh: " + std::string(importer.GetErrorString()));
        }

        auto node = scene->mRootNode->FindNode(rootNode.data());

        printNodeHierarchy(node);
    }
}

std::string readTextFile(std::filesystem::path path)
{
    std::ifstream file(path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

Texture readTextureFile(std::filesystem::path path)
{
    Texture result;
    int channels;
    stbi_uc *pixels = stbi_load(path.string().data(), &result.width, &result.height, &channels, STBI_rgb_alpha);
    result.pixels.resize(result.width * result.height);
    std::copy(pixels, pixels + result.width * result.height * 4,
              reinterpret_cast<unsigned char *>(result.pixels.data()));
    stbi_image_free(pixels);
    return std::move(result);
}
} // namespace

AssetManager::AssetManager(AssetOptions options) : _options(std::move(options))
{
}

AssetManager::~AssetManager() noexcept
{
}

AssetContainer::Ptr AssetManager::LoadAssets(std::filesystem::path path)
{

    try
    {
        std::filesystem::path filePath(path);
        auto extension = filePath.extension().string();
        auto containerName = filePath.replace_extension();

        std::vector<Camera> cameras;
        std::vector<Light> lights;
        std::vector<Model> models;
        std::vector<Material> materials;

        if (extension == ".json")
        {

            std::filesystem::path filePath(path);
            std::ifstream file(filePath);
            auto config = json::parse(file);
            filePath.remove_filename();

            if (config["type"] == "Model")
            {
                std::string rootNode = config["rootNode"];
                std::string filePath = config["file"];

                loadModel(filePath, rootNode);
            }
            else if (config["type"] == "ShaderMaterial")
            {

                Material material{AssetId::Create()};
                if (!config.contains("fragmentShaders") || !config.contains("vertexShaders"))
                {
                    throw AssetError("Material file missing required fields: " + filePath.string());
                }

                for (auto vertexShaderPath : config["vertexShaders"])
                {
                    std::cout << "vertex shader: " << vertexShaderPath << std::endl;
                    auto path = std::filesystem::proximate(filePath / vertexShaderPath);
                    material.vertexShaders.emplace_back(std::move(readTextFile(path)));
                }

                for (auto fragmentShaderPath : config["fragmentShaders"])
                {
                    std::cout << "fragment shader: " << fragmentShaderPath << std::endl;
                    auto path = std::filesystem::proximate(filePath / fragmentShaderPath);
                    material.fragmentShaders.emplace_back(std::move(readTextFile(path)));
                }

                for (auto texturePath : config["textures"])
                {
                    auto path = std::filesystem::proximate(filePath / texturePath);
                    material.textures.emplace_back(std::move(readTextureFile(path)));
                }

                materials.emplace_back(std::move(material));
            }
            else
            {
                throw AssetError("Unsupported material type: " + config["type"].get<std::string>());
            }
        }
        else if (Assimp::Importer importer; importer.IsExtensionSupported(extension))
        {

            const aiScene *scene = importer.ReadFile(path.string(), aiProcess_Triangulate);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                throw AssetError("Failed to load mesh: " + std::string(importer.GetErrorString()));
            }

            printNodeHierarchy(scene->mRootNode);

            for (unsigned int i = 0; i < scene->mNumCameras; i++)
            {
                const aiCamera *camera = scene->mCameras[i];
                cameras.emplace_back(Camera{
                    .id = AssetId::Create(),
                    .name = camera->mName.C_Str(),
                    .position = glm::vec3(camera->mPosition.x, camera->mPosition.y, camera->mPosition.z),
                    .target = glm::vec3(camera->mLookAt.x, camera->mLookAt.y, camera->mLookAt.z),
                    .up = glm::vec3(camera->mUp.x, camera->mUp.y, camera->mUp.z),
                    .fov = camera->mHorizontalFOV,
                    .nearClip = camera->mClipPlaneNear,
                    .farClip = camera->mClipPlaneFar,
                });
            }

            for (unsigned int i = 0; i < scene->mNumLights; i++)
            {
                const aiLight *light = scene->mLights[i];
                lights.emplace_back(Light{
                    .id = AssetId::Create(),
                    .name = light->mName.C_Str(),
                    .position = glm::vec3(light->mPosition.x, light->mPosition.y, light->mPosition.z),
                    .colorAmbient =
                        glm::vec4(light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b, 1.0),
                    .colorDiffuse =
                        glm::vec4(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b, 1.0),
                    .colorSpecular =
                        glm::vec4(light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b, 1.0),
                });
            }

            for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i)
            {
                const auto &node = *scene->mRootNode->mChildren[i];
                ModelBuilder modelBuilder;
                modelBuilder.SetName(node.mName.C_Str());
                loadModel(*scene, node, modelBuilder);
                models.emplace_back(modelBuilder.Build());
            }

            // for (unsigned int i = 0; i < scene->mNumMeshes; i++)
            // {

            //     auto mesh = scene->mMeshes[i];
            //     auto vertices = std::vector<Vertex>(mesh->mNumVertices);
            //     for (unsigned int j = 0; j < mesh->mNumVertices; j++)
            //     {
            //         Vertex vertex{
            //             .position = glm::vec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z),
            //             .normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z),
            //             .texCoord = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y)};
            //         vertices.push_back(vertex);
            //     }

            //     auto indices = std::vector<unsigned int>(mesh->mNumFaces * 3);
            //     for (unsigned int j = 0; j < mesh->mNumFaces; j++)
            //     {
            //         aiFace face = mesh->mFaces[j];
            //         indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
            //     }
            // }

            // std::deque<aiNode *> fringe;
            // fringe.push_back(scene->mRootNode);

            // while (!fringe.empty())
            // {
            //     auto node = fringe.front();
            //     fringe.pop_front();
            //     auto index = container.transforms.size();
            //     const auto &m = node->mTransformation;
            //     glm::mat4 transform(
            //         m[0][1], m[0][2], m[0][3], m[0][4],
            //         m[1][1], m[1][2], m[1][3], m[1][4],
            //         m[2][1], m[2][2], m[2][3], m[2][4],
            //         m[3][1], m[3][2], m[3][3], m[3][4]);
            //     container.transforms.push_back(Transform{
            //         .name = node->mName.C_Str(),
            //         .transform = transform,
            //     });

            //     for (unsigned int i = 0; i < node->mNumChildren; ++i)
            //     {
            //         fringe.push_back(node->mChildren[i]);
            //     }
            // }
        }
        return AssetContainer::Create(containerName.string(), std::move(cameras), std::move(lights), std::move(models),
                                      std::move(materials));
    }
    catch (const std::exception &e)
    {
        throw AssetError("Failed to load asset: "s + e.what());
    }
}

ModelBuilder &ModelBuilder::SetName(std::string name)
{
    _name = name;
    return *this;
}

ModelBuilder &ModelBuilder::AddVertex(Vertex vertex)
{
    _vertices.push_back(std::move(vertex));
    return *this;
}

ModelBuilder &ModelBuilder::AddIndex(uint32_t index)
{
    _indices.push_back(std::move(index));
    return *this;
}

ModelBuilder &ModelBuilder::AddTransform(ModelTransform transform)
{
    _transforms.push_back(std::move(transform));
    return *this;
}

ModelBuilder &ModelBuilder::AddPart(ModelPart part)
{
    _parts.push_back(std::move(part));
    return *this;
}

Model ModelBuilder::Build()
{
    return Model{
        .id = AssetId::Create(),
        .name = std::move(_name),
        .vertices = std::move(_vertices),
        .indices = std::move(_indices),
        .transforms = std::move(_transforms),
        .parts = std::move(_parts),
    };
}
