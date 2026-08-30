#pragma once

#include <engine/state.h>

#include <stdexcept>
#include <string>
#include <memory>

class InputError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct InputOptions
{
};

class InputSystem
{
public:
    InputSystem(InputOptions = {});
    ~InputSystem() noexcept;

    void Update(GameState &state);

private:
    struct impl;
    std::shared_ptr<impl> _impl;
    InputOptions _options;
};