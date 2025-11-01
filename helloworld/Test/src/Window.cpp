#include "Window.h"
#include <iostream>

Window::Window() : window(nullptr), context(nullptr), width(1280), height(720), scale(1), vsync(true)
{
    std::cout << "Window Constructor" << std::endl;
}

Window::~Window()
{
}

bool Window::Start()
{
    std::cout << "Init SDL3 Window" << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(
        "SDL3 OpenGL Window",
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr)
    {
        std::cerr << "Window creation failed! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    context = SDL_GL_CreateContext(window);
    if (context == nullptr)
    {
        std::cerr << "OpenGL context creation failed! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(vsync ? 1 : 0); // Enable/disable vsync

    // Update width/height from actual window
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    width = w;
    height = h;

    return true;
}

bool Window::Update()
{
    return true;
}

bool Window::PostUpdate()
{
    Render();
    return true;
}

void Window::Render()
{
    SDL_GL_SwapWindow(window);
}

bool Window::CleanUp()
{
    std::cout << "Destroying SDL Window" << std::endl;

    if (context != nullptr)
    {
        SDL_GL_DestroyContext(context);
        context = nullptr;
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
    return true;
}

void Window::GetWindowSize(int& width, int& height) const
{
    width = this->width;
    height = this->height;
}

int Window::GetScale() const
{
    return scale;
}

void Window::SetWindowSize(int w, int h)
{
    if (window)
    {
        SDL_SetWindowSize(window, w, h);
        width = w;
        height = h;
    }
}

void Window::SetVSync(bool enabled)
{
    vsync = enabled;
    // Apply to current context
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}