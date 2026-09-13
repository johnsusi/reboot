#include <engine/asset.h>
#include <engine/input.h>
#include <engine/video.h>

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        auto assetManager = AssetManager{};
        auto videoSystem = VideoSystem{};
        auto inputSystem = InputSystem{};

        auto material = assetManager.LoadAssets("data/assets/material.json");

        auto scene = assetManager.LoadAssets("scene.fbx");

        auto state = GameState{};
        state.material = material->GetMaterial("");
        state.entities.push_back(Entity{.camera = CameraComponent{scene->GetCamera("Camera")}});
        state.entities.push_back(Entity{.meshRenderer = MeshRenderComponent{true, scene->GetModel("Cube")}});

        while (state.running)
        {
            inputSystem.Update(state);
            videoSystem.Update(state);
        }
    }
    catch (const std::exception &err)
    {
        std::cerr << "Unexpected error: " << err.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unexpected error" << std::endl;
    }
    return 0;
}