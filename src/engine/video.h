#pragma once

#include <engine/state.h>

#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

struct DisplayMode
{
    std::string name;
    int width;
    int height;
    double frequency;
};

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

class DisplayModes
{
  public:
    DisplayModes(std::vector<DisplayMode> displayModes) : _displayModes(std::move(displayModes))
    {
    }
    const DisplayMode &Current() const noexcept;
    auto List() const noexcept -> std::span<DisplayMode>
    {
        return {_displayModes};
    }

  private:
    DisplayMode _current;
    std::vector<DisplayMode> _displayModes;
};

class VideoSystem
{
  public:
    VideoSystem(VideoOptions = {});
    ~VideoSystem() noexcept;

    void Update(GameState &state);

    auto ListDisplayModes() noexcept -> DisplayModes;

  private:
    struct impl;
    std::shared_ptr<impl> _impl;
    VideoOptions _options;
};