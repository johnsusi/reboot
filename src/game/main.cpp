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
                              << displayMode.refreshRate << "Hz" << std::endl;
                }
            }
        }

        auto material = assetManager.LoadAssets("data/assets/material.json");

        auto scene = assetManager.LoadAssets("scene.fbx");

        auto state = GameState{};
        state.material = material->GetMaterial("");
        state.entities.push_back(Entity{.camera = CameraComponent{scene->GetCamera("Camera")}});
        state.entities.push_back(Entity{.meshRenderer = MeshRenderComponent{true, scene->GetModel("Cube")}});

        // while (state.running)
        // {
        //     inputSystem.Update(state);
        //     videoSystem.Update(state);
        // }
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