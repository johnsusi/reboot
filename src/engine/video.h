#pragma once

#include <engine/state.h>

#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

class VideoError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct VideoOptions
{
    std::string title;
    uint32_t width = 1920;
    uint32_t height = 1080;
    double aspectRatio = 16.0 / 9.0;
};

struct RefreshRate
{
    int numerator;
    int denominator;
};

struct DisplayMode
{
    int width;
    int height;

    float pixelDensity;
    float refreshRate;
    RefreshRate refreshRateExact;
};

struct Display
{
    std::string name;
    std::vector<DisplayMode> modes;
    DisplayMode currentMode;
};

struct DisplayInfo
{
    Display currentDisplay;
    std::vector<Display> displays;
};

class VideoSystem
{
  public:
    VideoSystem(VideoOptions = {});
    ~VideoSystem() noexcept;

    void Update(GameState &state);

    auto GetDisplayInfo() noexcept -> DisplayInfo;

  private:
    struct impl;
    std::shared_ptr<impl> _impl;
    VideoOptions _options;
};