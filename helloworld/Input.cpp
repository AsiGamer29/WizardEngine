#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "SDL3/SDL.h"

#define MAX_KEYS 300

Input::Input() : Module()
{
    name = "input";

    keyboard = new KeyState[MAX_KEYS];
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);

    // Inicializar windowEvents a false
    for (int i = 0; i < WE_COUNT; ++i)
        windowEvents[i] = false;

    mouseMotionX = 0;
    mouseMotionY = 0;
    mouseX = 0;
    mouseY = 0;
}

Input::~Input()
{
    delete[] keyboard;
}

bool Input::Awake()
{
    // En SDL3 ya no existe SDL_InitSubSystem, solo inicializamos eventos si no están activos
    if (SDL_WasInit(SDL_INIT_EVENTS) == 0)
    {
        if (SDL_Init(SDL_INIT_EVENTS) < 0)
        {
            SDL_Log("Error initializing SDL events: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}

bool Input::Start()
{
    // SDL_StopTextInput ya no existe en SDL3
    return true;
}

bool Input::PreUpdate()
{
    static SDL_Event event;

    mouseMotionX = 0;
    mouseMotionY = 0;

    for (int i = 0; i < WE_COUNT; ++i)
        windowEvents[i] = false;

    // Reiniciar estados de teclado
    for (int i = 0; i < MAX_KEYS; ++i)
    {
        if (keyboard[i] == KEY_DOWN)
            keyboard[i] = KEY_REPEAT;
        else if (keyboard[i] == KEY_UP)
            keyboard[i] = KEY_IDLE;
    }

    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
    {
        if (mouseButtons[i] == KEY_DOWN)
            mouseButtons[i] = KEY_REPEAT;
        if (mouseButtons[i] == KEY_UP)
            mouseButtons[i] = KEY_IDLE;
    }

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            windowEvents[WE_QUIT] = true;
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
            if (event.button.button > 0 && event.button.button <= NUM_MOUSE_BUTTONS)
                mouseButtons[event.button.button - 1] = KEY_DOWN;
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button > 0 && event.button.button <= NUM_MOUSE_BUTTONS)
                mouseButtons[event.button.button - 1] = KEY_UP;
            break;

        case SDL_EVENT_MOUSE_MOTION:
        {
            int scale = Engine::GetInstance().window.get()->GetScale();
            mouseMotionX = event.motion.xrel / scale;
            mouseMotionY = event.motion.yrel / scale;
            mouseX = event.motion.x / scale;
            mouseY = event.motion.y / scale;
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
            // Si necesitas manejar la rueda del ratón, añade aquí tu lógica
            break;
        }
    }

    return true;
}


bool Input::CleanUp()
{
    // SDL3 ya no tiene SDL_QuitSubSystem
    return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
    return windowEvents[ev];
}

Vector2D Input::GetMousePosition()
{
    return Vector2D(mouseX, mouseY);
}

Vector2D Input::GetMouseMotion()
{
    return Vector2D(mouseMotionX, mouseMotionY);
}
