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

        assetManager.LoadAssets("data/assets/pill.json");

        videoSystem.ListDisplayModes();

        auto state = GameState{};

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