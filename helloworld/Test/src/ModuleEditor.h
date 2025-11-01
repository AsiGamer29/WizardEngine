#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <string>

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor() override;

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    void ProcessEvent(const SDL_Event& event);

private:
    bool show_demo_window = true;
    bool show_test_window = true;
    bool show_about_window = false; // About window toggle

    // Geometry loading menu helper
    std::string requested_geometry; // name of the geometry requested to load
};