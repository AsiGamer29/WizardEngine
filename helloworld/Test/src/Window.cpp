#include "Window.h"
#include <iostream>

Window::Window() : window(nullptr), width(1280), height(720), scale(1)
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


    // Create window WITH OpenGL flag
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

    return true;
}

bool Window::Update()
{
    /*SDL_Event event;*/

    // Poll events
    //while (SDL_PollEvent(&event))
    //{
    //    if (event.type == SDL_EVENT_QUIT)
    //    {
    //        return false;
    //    }

    //    // Handle window close button
    //    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    //    {
    //        return false;
    //    }
    //}

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