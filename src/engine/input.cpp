#include "input.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cstdlib>

#include <iostream>

using namespace std::literals;

struct InputSystem::impl
{
};

InputSystem::InputSystem(InputOptions options) : _options(std::move(options)), _impl(std::make_shared<impl>())
{
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS))
        throw InputError("Unable to initialize SDL: "s + SDL_GetError());
}

InputSystem::~InputSystem() noexcept
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

void InputSystem::Update(GameState &state)
{

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            /* fallthrough */
        case SDL_EVENT_QUIT:
            state.running = false;
            break;
        default:
            break;
        }
    }
}