#pragma once
#include "Module.h"
#include <SDL3/SDL.h>

class Window : public Module
{
public:
    Window();
    ~Window();

    bool Start() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;
    bool vsync = true; 
    void Render();
    void GetWindowSize(int& width, int& height) const;
    int GetScale() const;

    SDL_Window* GetWindow() const { return window; }
    SDL_GLContext GetContext() const { return context; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }


    // New: allow runtime changes from editor
    void SetWindowSize(int w, int h);
    void SetVSync(bool enabled);
private:
    SDL_Window* window;
    SDL_GLContext context;
    int width;
    int height;
    int scale;
    bool vsync = true; // track current vsync state

};