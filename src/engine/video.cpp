#include "video.h"

#include <print>

#include <SDL3/SDL.h>
#include <glad/glad.h>

using namespace std::literals;

struct VideoSystem::impl
{
    SDL_Window *window = nullptr;
    SDL_GLContext context = nullptr;
};

VideoSystem::VideoSystem(VideoOptions options) : _options(std::move(options)), _impl(std::make_shared<impl>())
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        throw VideoError("Unable to initialize SDL: "s + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    _impl->window = SDL_CreateWindow(options.title.data(), options.width, options.height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!_impl->window)
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize SDL Window: "s + SDL_GetError());
    }

    _impl->context = SDL_GL_CreateContext(_impl->window);

    if (!_impl->context)
    {
        SDL_DestroyWindow(_impl->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize SDL OpenGL Context: "s + SDL_GetError());
    }

    int version = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress));
    if (!version)
    {
        SDL_GL_DestroyContext(_impl->context);
        SDL_DestroyWindow(_impl->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize GLAD OpenGL context");
    }
}

VideoSystem::~VideoSystem() noexcept
{
    if (_impl->context)
    {
        SDL_GL_DestroyContext(_impl->context);
        _impl->context = nullptr;
    }
    if (_impl->window)
    {
        SDL_DestroyWindow(_impl->window);
        _impl->window = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VideoSystem::Update(GameState &state)
{
}

// auto VideoSystem::GetCurrentDisplayMode() noexcept -> DisplayMode
// {
//     for (int index = 0; index < displayCount; ++index)
//     {
//         SDL_DisplayID display = displays[index];

//         const char *name = SDL_GetDisplayName(display);
//         const SDL_DisplayMode *desktopMode =
//             SDL_GetDesktopDisplayMode(display);
//         const SDL_DisplayMode *currentMode =
//             SDL_GetCurrentDisplayMode(display);
// }

auto VideoSystem::ListDisplayModes() noexcept -> std::vector<DisplayMode>
{
    std::vector<DisplayMode> result;

    int displayCount = 0;
    const SDL_DisplayID *displays = SDL_GetDisplays(&displayCount);

    for (int index = 0; index < displayCount; ++index)
    {
        SDL_DisplayID display = displays[index];

        std::string name = SDL_GetDisplayName(display);

        int modeCount = 0;
        SDL_DisplayMode **modes =
            SDL_GetFullscreenDisplayModes(display, &modeCount);

        if (!modes)
            continue;

        for (int modeIndex = 0; modeIndex < modeCount; ++modeIndex)
        {
            const SDL_DisplayMode *mode = modes[modeIndex];
            result.emplace_back(name, mode->w, mode->h, mode->refresh_rate);
        }

        SDL_free(modes);
    }

    return result;
}

class Shader
{
public:
    Shader(const char *source, GLenum shaderType)
    {
        _shader = glCreateShader(shaderType);
        glShaderSource(_shader, 1, &source, nullptr);
        glCompileShader(_shader);

        GLint success = 0;
        glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE)
        {
            GLint length = 0;
            glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
            std::string infoLog(' ', length);
            glGetShaderInfoLog(_shader, length, nullptr, infoLog.data());
            throw VideoError(std::string("Shader compilation failed: ") + infoLog);
        }
    }

    ~Shader()
    {
        if (_shader)
        {
            glDeleteShader(_shader);
            _shader = 0;
        }
    }

    operator GLuint() const
    {
        return _shader;
    }

private:
    GLuint _shader = 0;
};

class ShaderProgram
{
public:
    ShaderProgram(const char *vertexSource, const char *fragmentSource)
    {

        auto vertexShader = Shader(vertexSource, GL_VERTEX_SHADER);
        auto fragmentShader = Shader(fragmentSource, GL_FRAGMENT_SHADER);

        _program = glCreateProgram();
        glAttachShader(_program, vertexShader);
        glAttachShader(_program, fragmentShader);
        glLinkProgram(_program);

        GLint success = 0;
        glGetProgramiv(_program, GL_LINK_STATUS, &success);
        if (success != GL_TRUE)
        {
            GLint length = 0;
            glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
            std::string infoLog(' ', length);
            glGetProgramInfoLog(_program, length, nullptr, infoLog.data());

            throw VideoError(std::string("Shader program linking failed: ") + infoLog);
        }
    }

    ~ShaderProgram()
    {
        if (_program)
        {
            glDeleteProgram(_program);
            _program = 0;
        }
    }

    operator GLuint() const
    {
        return _program;
    }

private:
    GLuint _program = 0;
};