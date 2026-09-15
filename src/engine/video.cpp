#include "video.h"
#include "asset.h"

#include <core/factory.h>

#include <iostream>
#include <map>
#include <memory>
#include <print>
#include <vector>

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

using namespace std::literals;

namespace
{

class Shader : public Managed<Shader>
{
  public:
    Shader(Factory, const char *source, GLenum shaderType)
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

class ShaderProgram : public Managed<ShaderProgram>
{
  public:
    ShaderProgram(Factory, std::string_view vertexSource, std::string_view fragmentSource)
    {

        auto vertexShader = Shader::Create(vertexSource.data(), GL_VERTEX_SHADER);
        auto fragmentShader = Shader::Create(fragmentSource.data(), GL_FRAGMENT_SHADER);

        _program = glCreateProgram();
        glAttachShader(_program, *vertexShader);
        glAttachShader(_program, *fragmentShader);
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

    void Use()
    {
        glUseProgram(_program);
    }

  private:
    GLuint _program = 0;
};

} // namespace

struct CompiledMaterial
{
    ShaderProgram::Ptr program;
};

struct CompiledModel
{
    const GLuint material;
    const GLuint vao;
    const GLsizei numVertices;
    const GLsizei numIndices;
};

class RenderTarget : Managed<RenderTarget>
{
  public:
    constexpr RenderTarget() = default;

    RenderTarget(Factory, int width, int height) : _width(width), _height(height)
    {
        Create();
    }

    ~RenderTarget()
    {
        Destroy();
    }

    void EnsureSize(int width, int height)
    {
        if (width == _width && height == _height)
            return;
        Destroy();
        Create();
    }

    void Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    }

    void Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint GetColorTexture() const
    {
        return _colorTexture;
    }

    GLuint GetDepthBuffer() const
    {
        return _depthBuffer;
    }

  protected:
    void Create()
    {

        glGenFramebuffers(1, &_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

        glGenTextures(1, &_colorTexture);
        glBindTexture(GL_TEXTURE_2D, _colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture, 0);

        glGenRenderbuffers(1, &_depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw VideoError("Render target framebuffer is incomplete");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Destroy()
    {
        if (!_framebuffer && !_colorTexture && !_depthBuffer)
            return;

        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (_framebuffer)
        {
            glDeleteFramebuffers(1, &_framebuffer);
            _framebuffer = 0;
        }

        if (_depthBuffer)
        {
            glDeleteRenderbuffers(1, &_depthBuffer);
            _depthBuffer = 0;
        }

        if (_colorTexture)
        {
            glDeleteTextures(1, &_colorTexture);
            _colorTexture = 0;
        }
    }

  private:
    GLuint _framebuffer = 0;
    GLuint _colorTexture = 0;
    GLuint _depthBuffer = 0;
    int _width = 0;
    int _height = 0;
};

struct VideoSystem::impl
{
    SDL_Window *window = nullptr;
    SDL_GLContext glContext = nullptr;
    ImGuiContext *imguiContext;
    RenderTarget sceneTarget;
    GLuint debugQuadVao = 0;
    GLuint debugQuadVbo = 0;
    GLuint debugQuadEbo = 0;

    std::map<AssetId, CompiledMaterial> materials;
    std::map<AssetId, CompiledModel> models;

    void EnsureDebugQuad()
    {
        if (debugQuadVao)
            return;

        constexpr float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f,
        };
        constexpr unsigned int indices[] = {0, 1, 2, 2, 3, 0};

        glGenVertexArrays(1, &debugQuadVao);
        glGenBuffers(1, &debugQuadVbo);
        glGenBuffers(1, &debugQuadEbo);

        glBindVertexArray(debugQuadVao);
        glBindBuffer(GL_ARRAY_BUFFER, debugQuadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugQuadEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    CompiledMaterial GetCompiledMaterial(const std::shared_ptr<const Material> &material)
    {
        if (auto it = materials.find(material->id); it != materials.end())
        {
            return it->second;
        }

        std::cout << "id: " << material->id.get() << ", size: " << material->vertexShaders.size() << ", "
                  << material->fragmentShaders.size() << std::endl;
        auto program = ShaderProgram::Create(material->vertexShaders.front(), material->fragmentShaders.front());
        auto compiledMaterial = CompiledMaterial{program};
        materials.emplace(material->id, compiledMaterial);
        return compiledMaterial;
    }

    CompiledModel GetCompiledModel(const std::shared_ptr<const Model> &model)
    {

        if (auto it = models.find(model->id); it != models.end())
        {
            return it->second;
        }

        GLuint vao, vbo, ebo;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(model->vertices.size() * sizeof(Vertex)),
                     model->vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->indices.size() * sizeof(unsigned int), model->indices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, position)));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, normal)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void *>(offsetof(Vertex, texCoord)));

        glBindVertexArray(0);
        auto compiledModel = CompiledModel{0, vao, static_cast<GLsizei>(model->vertices.size()),
                                           static_cast<GLsizei>(model->indices.size())};
        models.emplace(model->id, compiledModel);
        return compiledModel;
    }

    void RenderScene(GameState &state)
    {

        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        glm::mat4 model = glm::mat4(1.0f);

        // auto &transform = state.GetComponent<TransformComponent>(state.activeCameraId);
        // auto &camera = state.GetComponent<CameraComponent>(state.activeCameraId);
        // view = camera.getViewMatrix(transform);
        // projection = camera.getProjectionMatrix();
        // std::cout << "Camera ID: " << state.activeCameraId << std::endl;
        // std::cout << "View: " << glm::to_string(view) << std::endl;
        // std::cout << "Projection: " << glm::to_string(projection) << std::endl;
        // }

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.45f, 0.55f, 0.60f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto material = GetCompiledMaterial(state.material);
        glUseProgram(*material.program);

        GLint uModelLoc = glGetUniformLocation(*material.program, "uModel");
        GLint uViewLoc = glGetUniformLocation(*material.program, "uView");
        GLint uProjectionLoc = glGetUniformLocation(*material.program, "uProjection");
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        EnsureDebugQuad();
        glBindVertexArray(debugQuadVao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

VideoSystem::VideoSystem(VideoOptions options) : _options(std::move(options)), _impl(std::make_shared<impl>())
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        throw VideoError("Unable to initialize SDL: "s + SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    _impl->window = SDL_CreateWindow(options.title.data(), options.width * main_scale, options.height * main_scale,
                                     SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!_impl->window)
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize SDL Window: "s + SDL_GetError());
    }

    _impl->glContext = SDL_GL_CreateContext(_impl->window);

    if (!_impl->glContext)
    {
        SDL_DestroyWindow(_impl->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize SDL OpenGL Context: "s + SDL_GetError());
    }

    int version = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress));
    if (!version)
    {
        SDL_GL_DestroyContext(_impl->glContext);
        SDL_DestroyWindow(_impl->window);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        throw VideoError("Unable to initialize GLAD OpenGL context");
    }

    IMGUI_CHECKVERSION();
    _impl->imguiContext = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling,
                                     // changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale; // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true
                                     // automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true; // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor
                                   // DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true; // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL3_InitForOpenGL(_impl->window, _impl->glContext);
    // if (options.useOpenGLES)
    // {
    //     ImGui_ImplOpenGL3_Init("#version 300 es");
    // }
    // else
    // {
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // }
}

VideoSystem::~VideoSystem() noexcept
{
    if (_impl->imguiContext)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(_impl->imguiContext);
        _impl->imguiContext = nullptr;
    }

    if (_impl->glContext)
    {
        SDL_GL_DestroyContext(_impl->glContext);
        _impl->glContext = nullptr;
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

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();

    // for (auto &&entity : state.entities)
    // {
    //     if (entity.meshRenderer && entity.meshRenderer->visible)
    //     {
    //         auto compiledModel = _impl->GetCompiledModel(entity.meshRenderer->model);

    //         // if (entity.transform)
    //         // {
    //         //     auto model = glm::mat4(1.0f);
    //         //     model = glm::translate(model, entity.transform->position);
    //         //     model = glm::rotate(model, entity.transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    //         //     model = glm::rotate(model, entity.transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    //         //     model = glm::rotate(model, entity.transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    //         //     model = glm::scale(model, entity.transform->scale);
    //         //     glUniformMatrix4fv(glGetUniformLocation(cachedEntry.material, "model"), 1, GL_FALSE,
    //         //                        glm::value_ptr(model));
    //         // }

    //         // if (entity.rigidBody)
    //         // {
    //         //     btTransform transform;
    //         //     entity.rigidBody->body->getMotionState()->getWorldTransform(transform);
    //         //     auto position = transform.getOrigin();
    //         //     auto rotation = transform.getRotation();
    //         //     auto model = glm::mat4(1.0f);
    //         //     model = glm::translate(model, glm::vec3(position.x(), position.y(), position.z()));
    //         //     // model = glm::rotate(model, glm::angle(rotation), glm::vec3(rotation.x(), rotation.y(),
    //         //     // rotation.z()));
    //         //     glUniformMatrix4fv(glGetUniformLocation(entity.meshRender->material, "model"), 1, GL_FALSE,
    //         //                        glm::value_ptr(model));
    //         // }
    //         // glUseProgram(meshRenderer.materialId);
    //         std::cout << "uniforms: " << uModelLoc << ", " << uViewLoc << ", " << uProjectionLoc << std::endl;
    //         std::cout << compiledModel.numIndices << std::endl;

    //         glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    //         glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    //         glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //         glBindVertexArray(compiledModel.vao);
    //         glDrawElements(GL_TRIANGLES, compiledModel.numIndices, GL_UNSIGNED_INT, 0);
    //         glBindVertexArray(0);
    //     }
    // }
    ImGuiIO &io = ImGui::GetIO();

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
    ImGui::Text("This is some useful text.");
    ImGui::End();
    ImGui::Begin("Scene");

    constexpr float sceneAspect = 16.0f / 9.0f;

    auto available = ImGui::GetContentRegionAvail();
    auto imageSize = available;

    if (available.x / available.y > sceneAspect)
    {
        // Window is too wide: use full height.
        imageSize.x = available.y * sceneAspect;
    }
    else
    {
        // Window is too tall: use full width.
        imageSize.y = available.x / sceneAspect;
    }

    ImVec2 imagePos = ImGui::GetCursorPos();
    imagePos.x += (available.x - imageSize.x) * 0.5f;
    imagePos.y += (available.y - imageSize.y) * 0.5f;
    ImGui::SetCursorPos(imagePos);

    float dpiScale = io.DisplayFramebufferScale.x;

    int renderWidth = std::max(1, static_cast<int>(std::ceil(imageSize.x * dpiScale)));
    int renderHeight = std::max(1, static_cast<int>(std::ceil(imageSize.y * dpiScale)));

    _impl->sceneTarget.EnsureSize(renderWidth, renderHeight);

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);

    // auto &transform = state.GetComponent<TransformComponent>(state.activeCameraId);
    // auto &camera = state.GetComponent<CameraComponent>(state.activeCameraId);
    // view = camera.getViewMatrix(transform);
    // projection = camera.getProjectionMatrix();
    // std::cout << "Camera ID: " << state.activeCameraId << std::endl;
    // std::cout << "View: " << glm::to_string(view) << std::endl;
    // std::cout << "Projection: " << glm::to_string(projection) << std::endl;
    // }

    _impl->sceneTarget.Bind();
    glViewport(0, 0, renderWidth, renderHeight);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.45f, 0.55f, 0.60f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto material = _impl->GetCompiledMaterial(state.material);
    glUseProgram(*material.program);

    GLint uModelLoc = glGetUniformLocation(*material.program, "uModel");
    GLint uViewLoc = glGetUniformLocation(*material.program, "uView");
    GLint uProjectionLoc = glGetUniformLocation(*material.program, "uProjection");
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    _impl->EnsureDebugQuad();
    glBindVertexArray(_impl->debugQuadVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Image(_impl->sceneTarget.GetColorTexture(), imageSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    ImGui::End();

    ImGui::Render();

    // First draw scene to texture

    // Then render UI to view
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
    SDL_GL_SwapWindow(_impl->window);
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

auto VideoSystem::GetDisplayInfo() noexcept -> DisplayInfo
{
    DisplayInfo result;

    int displayCount = 0;
    auto displays = SDL_GetDisplays(&displayCount);

    for (int index = 0; index < displayCount; ++index)
    {

        Display display;

        SDL_DisplayID id = displays[index];

        display.name = SDL_GetDisplayName(id);

        int modeCount = 0;
        auto modes = SDL_GetFullscreenDisplayModes(id, &modeCount);

        if (!modes)
            continue;

        for (int modeIndex = 0; modeIndex < modeCount; ++modeIndex)
        {
            auto mode = modes[modeIndex];
            display.modes.emplace_back(mode->w, mode->h, mode->pixel_density, mode->refresh_rate,
                                       RefreshRate{mode->refresh_rate_numerator, mode->refresh_rate_denominator});
        }

        SDL_free(modes);

        result.displays.emplace_back(std::move(display));
    }

    SDL_free(displays);
    return {std::move(result)};
}

// ImGui_ImplOpenGL3_NewFrame();
//     ImGui_ImplSDL3_NewFrame();

//     ImGui::NewFrame();
//     ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
//     ImGui::Text("This is some useful text.");
//     ImGui::End();

//     ImGui::Render();
//     ImGuiIO &io = ImGui::GetIO();

//     glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
//     glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
//     glClear(GL_COLOR_BUFFER_BIT);

//     glm::mat4 view = glm::mat4(1.0f);
//     glm::mat4 projection = glm::mat4(1.0f);

//     if (state.activeCameraId)
//     {
//         auto &transform = state.GetComponent<TransformComponent>(state.activeCameraId);
//         auto &camera = state.GetComponent<CameraComponent>(state.activeCameraId);
//         view = camera.getViewMatrix(transform);
//         projection = camera.getProjectionMatrix();
//         std::cout << "Camera ID: " << state.activeCameraId << std::endl;
//         // std::cout << "View: " << glm::to_string(view) << std::endl;
//         // std::cout << "Projection: " << glm::to_string(projection) << std::endl;
//     }
//     else
//     {
//         std::cerr << "No active camera found!" << std::endl;
//     }

//     for (auto &entity : state.entities)
//     {
//         glm::mat4 model = glm::mat4(1.0f);

//         for (auto &component : entity.components)
//         {
//             if (std::holds_alternative<TransformComponent>(component))
//             {
//                 auto transform = std::get<TransformComponent>(component);
//                 model = transform.getMatrix();
//             }

//             if (std::holds_alternative<MeshRendererComponent>(component))
//             {
//                 auto meshRenderer = std::get<MeshRendererComponent>(component);
//                 glUseProgram(meshRenderer.materialId);
//                 glBindVertexArray(meshRenderer.meshId);
//                 GLint uModelLoc = glGetUniformLocation(meshRenderer.materialId, "uModel");
//                 GLint uViewLoc = glGetUniformLocation(meshRenderer.materialId, "uView");
//                 GLint uProjectionLoc = glGetUniformLocation(meshRenderer.materialId, "uProjection");

//                 glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
//                 glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
//                 glUniformMatrix4fv(uProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

//                 glDrawElements(GL_TRIANGLES, meshRenderer.count, GL_UNSIGNED_INT, 0);
//             }
//         }
//     }

//     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//     SDL_GL_SwapWindow(_impl->window);