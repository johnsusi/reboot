#include "asset.h"

#include <fstream>
#include <string>
#include <iostream>
#include <queue>

#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std::literals;

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
    std::cout << node->mTransformation.a1 << ", " << node->mTransformation.a2 << ", " << node->mTransformation.a3 << ", " << node->mTransformation.a4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.b1 << ", " << node->mTransformation.b2 << ", " << node->mTransformation.b3 << ", " << node->mTransformation.b4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.c1 << ", " << node->mTransformation.c2 << ", " << node->mTransformation.c3 << ", " << node->mTransformation.c4 << std::endl;
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
    std::cout << node->mTransformation.d1 << ", " << node->mTransformation.d2 << ", " << node->mTransformation.d3 << ", " << node->mTransformation.d4 << std::endl;

    // Recurse into children
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        printNodeHierarchy(node->mChildren[i], depth + 1);
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

AssetManager::AssetManager(AssetOptions options) : _options(std::move(options))
{
}

AssetManager::~AssetManager() noexcept
{
}

void AssetManager::LoadAssets(std::filesystem::path path)
{

    try
    {
        std::filesystem::path filePath(path);
        auto extension = filePath.extension().string();
        auto containerName = filePath.replace_extension();
        auto container = AssetContainer{.name = containerName};

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

            //     if (config["type"] == "ShaderMaterial")
            //     {

            //         if (!config.contains("fragmentShader") || !config.contains("vertexShader"))
            //         {
            //             throw AssetError("Material file missing required fields: " + filePath.string());
            //         }

            //         auto vertexShaderPath = std::filesystem::proximate(filePath / config["vertexShader"]);
            //         std::ifstream vertexShaderFile(vertexShaderPath);
            //         if (!vertexShaderFile.is_open())
            //         {
            //             throw AssetError("Failed to open vertex shader file: " + vertexShaderPath.string());
            //         }
            //         std::string vertexShaderSource((std::istreambuf_iterator<char>(vertexShaderFile)),
            //                                        std::istreambuf_iterator<char>());
            //         vertexShaderFile.close();

            //         auto fragmentShaderPath = std::filesystem::proximate(filePath / config["fragmentShader"]);

            //         std::ifstream fragmentShaderFile(fragmentShaderPath);
            //         if (!fragmentShaderFile.is_open())
            //         {
            //             throw AssetError("Failed to open fragment shader file: " + fragmentShaderPath.string());
            //         }
            //         std::string fragmentShaderSource((std::istreambuf_iterator<char>(fragmentShaderFile)),
            //                                          std::istreambuf_iterator<char>());
            //         fragmentShaderFile.close();

            //     }
            //     else
            //     {
            //         throw AssetError("Unsupported material type: " + config["type"].get<std::string>());
            //     }
        }
        else
        {
            Assimp::Importer importer;
            if (importer.IsExtensionSupported(extension))
            {
                const aiScene *scene = importer.ReadFile(std::string{path}, aiProcess_Triangulate);

                if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
                {
                    throw AssetError("Failed to load mesh: " + std::string(importer.GetErrorString()));
                }

                printNodeHierarchy(scene->mRootNode);

                for (unsigned int i = 0; i < scene->mNumCameras; i++)
                {
                    const aiCamera *camera = scene->mCameras[i];
                    container.cameras.push_back(Camera{
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
                    container.lights.push_back(Light{
                        .name = light->mName.C_Str(),
                        .position = glm::vec3(light->mPosition.x, light->mPosition.y, light->mPosition.z),
                        .colorAmbient = glm::vec4(light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b, 1.0),
                        .colorDiffuse = glm::vec4(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b, 1.0),
                        .colorSpecular = glm::vec4(light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b, 1.0),
                    });
                }

                for (unsigned int i = 0; i < scene->mNumMeshes; i++)
                {

                    auto mesh = scene->mMeshes[i];
                    auto vertices = std::vector<Vertex>(mesh->mNumVertices);
                    for (unsigned int j = 0; j < mesh->mNumVertices; j++)
                    {
                        Vertex vertex{
                            .position = glm::vec3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z),
                            .normal = glm::vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z),
                            .texCoord = glm::vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y)};
                        vertices.push_back(vertex);
                    }

                    auto indices = std::vector<unsigned int>(mesh->mNumFaces * 3);
                    for (unsigned int j = 0; j < mesh->mNumFaces; j++)
                    {
                        aiFace face = mesh->mFaces[j];
                        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
                    }

                    container.meshes.push_back(Mesh{
                        .name = mesh->mName.C_Str(),
                        .vertices = std::move(vertices),
                        .indices = std::move(indices),
                    });
                }

                std::deque<aiNode *> fringe;
                fringe.push_back(scene->mRootNode);

                while (!fringe.empty())
                {
                    auto node = fringe.front();
                    fringe.pop_front();
                    auto index = container.transforms.size();
                    const auto &m = node->mTransformation;
                    glm::mat4 transform(
                        m[0][1], m[0][2], m[0][3], m[0][4],
                        m[1][1], m[1][2], m[1][3], m[1][4],
                        m[2][1], m[2][2], m[2][3], m[2][4],
                        m[3][1], m[3][2], m[3][3], m[3][4]);
                    container.transforms.push_back(Transform{
                        .name = node->mName.C_Str(),
                        .transform = transform,
                    });

                    for (unsigned int i = 0; i < node->mNumChildren; ++i)
                    {
                        fringe.push_back(node->mChildren[i]);
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        throw AssetError("Failed to load asset: "s + e.what());
    }
}