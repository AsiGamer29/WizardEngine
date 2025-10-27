#pragma once
#include "Module.h"
#include "Window.h"
#include <SDL3/SDL.h>

#define MAX_KEYS 512
#define NUM_MOUSE_BUTTONS 5

enum KeyState
{
    KEY_IDLE = 0,
    KEY_DOWN,
    KEY_REPEAT,
    KEY_UP
};

enum WindowEvent
{
    WE_QUIT = 0,
    WE_HIDE,
    WE_SHOW,
    WE_COUNT
};

class Input : public Module
{
public:
    Input();
    ~Input();

    bool Awake() override;
    bool Start() override;
    bool PreUpdate() override;
    bool CleanUp() override;

    KeyState GetKey(int scancode) const;
    KeyState GetMouseButton(int id) const;

    SDL_Point GetMouseMotion() const { return SDL_Point{ mouseMotionX, mouseMotionY }; }
    int GetMouseWheel() const { return mouseWheelY; }
    int GetMouseX() const { return mouseX; }
    int GetMouseY() const { return mouseY; }

    bool GetWindowEvent(WindowEvent event) const {
        if (event < 0 || event >= WE_COUNT) return false;
        return windowEvents[event];
    }


private:
    KeyState keyboard[MAX_KEYS];
    KeyState mouseButtons[NUM_MOUSE_BUTTONS];
    bool windowEvents[WE_COUNT];

    int mouseX, mouseY;
    int mouseMotionX, mouseMotionY;
    int mouseWheelY;
};
