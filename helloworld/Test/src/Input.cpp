#include "Input.h"
#include <iostream>
#include <cstring>

Input::Input() : Module()
{
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
    memset(windowEvents, 0, sizeof(windowEvents));

    mouseX = mouseY = mouseMotionX = mouseMotionY = mouseWheelY = 0;
}

Input::~Input() {}

bool Input::Awake()
{
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
    {
        std::cerr << "SDL_EVENTS could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

bool Input::Start()
{
    return true;
}

bool Input::PreUpdate()
{
    SDL_Event event;

    // Limpiar archivos arrastrados este frame
    droppedFiles.clear();

    mouseMotionX = mouseMotionY = 0;
    mouseWheelY = 0;
    for (int i = 0; i < WE_COUNT; ++i)
        windowEvents[i] = false;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            windowEvents[WE_QUIT] = true;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.repeat == 0 && event.key.scancode < MAX_KEYS)
                keyboard[event.key.scancode] = KEY_DOWN;
            else if (event.key.scancode < MAX_KEYS)
                keyboard[event.key.scancode] = KEY_REPEAT;
            break;

        case SDL_EVENT_KEY_UP:
            if (event.key.scancode < MAX_KEYS)
                keyboard[event.key.scancode] = KEY_UP;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
                mouseButtons[event.button.button - 1] = KEY_DOWN;
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
                mouseButtons[event.button.button - 1] = KEY_UP;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            mouseMotionX = event.motion.xrel;
            mouseMotionY = event.motion.yrel;
            mouseX = event.motion.x;
            mouseY = event.motion.y;
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            mouseWheelY = event.wheel.y;
            break;

        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            windowEvents[WE_HIDE] = true;
            break;

        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            windowEvents[WE_SHOW] = true;
            break;

            // EVENTOS DE DRAG & DROP
        case SDL_EVENT_DROP_BEGIN:
            std::cout << "[Drag&Drop] Inicio del arrastre." << std::endl;
            break;

        case SDL_EVENT_DROP_FILE:
            if (event.drop.data)
            {
                std::string path = event.drop.data;
                droppedFiles.push_back(path);
                std::cout << "[Drag&Drop] Archivo soltado: " << path << std::endl;
            }
            break;

        case SDL_EVENT_DROP_TEXT:
            if (event.drop.data)
            {
                std::string text = event.drop.data;
                std::cout << "[Drag&Drop] Texto soltado: " << text << std::endl;
            }
            break;

        case SDL_EVENT_DROP_COMPLETE:
            std::cout << "[Drag&Drop] Arrastre completado." << std::endl;
            break;

        default:
            break;
        }
    }

    // Actualizar estados del teclado
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keyboard[i] == KEY_DOWN)
            keyboard[i] = KEY_REPEAT;
        else if (keyboard[i] == KEY_UP)
            keyboard[i] = KEY_IDLE;
    }

    // Actualizar estados del ratón
    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
    {
        if (mouseButtons[i] == KEY_DOWN)
            mouseButtons[i] = KEY_REPEAT;
        else if (mouseButtons[i] == KEY_UP)
            mouseButtons[i] = KEY_IDLE;
    }

    return true;
}

bool Input::CleanUp()
{
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    return true;
}

KeyState Input::GetKey(int scancode) const
{
    if (scancode < 0 || scancode >= MAX_KEYS)
        return KEY_IDLE;
    return keyboard[scancode];
}

KeyState Input::GetMouseButton(int id) const
{
    if (id < 1 || id > NUM_MOUSE_BUTTONS)
        return KEY_IDLE;
    return mouseButtons[id - 1];
}
