#include <engine/asset.h>
#include <engine/input.h>
#include <engine/video.h>

#include <iostream>
#include <map>

int main(int argc, char *argv[])
{
    try
    {
        auto assetManager = AssetManager{};
        auto videoSystem = VideoSystem{};
        auto inputSystem = InputSystem{};

        auto displayInfo = videoSystem.GetDisplayInfo();

        for (auto display : displayInfo.displays)
        {
            std::cout << "Display " << display.name << std::endl;
            std::map<int, std::vector<DisplayMode>> byF;
            for (auto &&displayMode : display.modes)
            {
                byF[std::lround(displayMode.refreshRate)].push_back(displayMode);
            }

            for (auto &&[f, displayModes] : byF)
            {
                std::cout << "    " << "Frequency " << f << "Hz" << std::endl;
                for (auto &&displayMode : displayModes)
                {
                    std::cout << "        " << displayMode.width << "x" << displayMode.height << " at "
                              << displayMode.refreshRate << "Hz" << " " << displayMode.refreshRateExact.numerator << ":"
                              << displayMode.refreshRateExact.denominator << std::endl;
                }
            }
        }

        auto material = assetManager.LoadAssets("data/assets/material.json");

        auto scene = assetManager.LoadAssets("data/assets/scene.fbx");

        auto state = GameState{};
        state.material = material->GetMaterial("");

        auto cameraId =
            state.CreateEntity([&](auto entity) { entity.camera = CameraComponent{scene->GetCamera("Camera")}; });
        state.CreateEntity([&](auto entity) {
            entity.meshRenderer = MeshRenderComponent{true, scene->GetModel("Cube")};
            entity.transform = TransformComponent{.scale = glm::vec3(0.1, 0.1, 0.1)};
        });

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