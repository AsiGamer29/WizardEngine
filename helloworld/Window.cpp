#include "Window.h"
#include "Engine.h"
#include "SDL3/SDL.h"

Window::Window() : Module()
{
    window = nullptr;
    name = "window";
}

Window::~Window() {}

bool Window::Awake()
{
    bool ret = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // Lee parámetros del config si existen
    bool fullscreen = configParameters.child("fullscreen").attribute("value").as_bool(false);
    bool borderless = configParameters.child("borderless").attribute("value").as_bool(false);
    bool resizable = configParameters.child("resizable").attribute("value").as_bool(true);
    bool fullscreen_window = configParameters.child("fullscreen_window").attribute("value").as_bool(false);

    width = configParameters.child("resolution").attribute("width").as_int(1280);
    height = configParameters.child("resolution").attribute("height").as_int(720);
    scale = configParameters.child("resolution").attribute("scale").as_int(1);

    // Crea ventana
    Uint32 window_flags = 0;
    if (resizable) window_flags |= SDL_WINDOW_RESIZABLE;

    window = SDL_CreateWindow("Platform Game", width, height, window_flags);

    if (!window)
    {
        SDL_Log("Error creating window: %s", SDL_GetError());
        return false;
    }

    // Aplica propiedades adicionales
    if (fullscreen_window)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    if (borderless)
        SDL_SetWindowBordered(window, false);

    return ret;
}

bool Window::CleanUp()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
    return true;
}

void Window::SetTitle(const char* new_title)
{
    if (window)
        SDL_SetWindowTitle(window, new_title);
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
