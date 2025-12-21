# Wizard Engine Documentation #
## Overview ##

Wizard Engine is an early in development 3D game engine designed for rapid prototyping and intuitive scene editing. It provides an easy space for creating, managing, and rendering 3D game objects with real-time camera control similar to professional engines like Unity.

With Wizard Engine, developers can:

- Create and manage Game Objects.

- Load and display 3D models.

- Apply textures and materials.

- Navigate scenes using a dynamic editor camera.

## Core Features ##
### Game Objects ###

Game Objects are the fundamental entities in Wizard Engine.
Each Game Object can:

- Contain transform data (position, rotation, scale).

- Contain mesh data (3D model).

- Contain material data (texture, color).

- Be organized hierarchically within a scene.

- They can me moved outside or inside a parent within a hierachy.

- You can create Empty GameObjects.

- You can create a camera GameObject.

### Model Loading ###

Wizard Engine supports importing 3D models in .fbx formats.

### Textures and Materials ###

Materials define the visual appearance of a model. Each material can include:

### Drag and Drop features ###

The user can drag a file from their file manager and drop it to the Engine. If it's a .fbx file, the model will load on the screen, and if it's a texture file, the texture will be placed in the model loaded.

The user can also drag a file from the asset manager and drop it to the Engine. If it's a .fbx file, the model will load on the screen, and if it's a texture file, the texture will be placed in the model selected.

### Resource Manager ###

The user can save and load scenes.

By drag and dropping files into the scene, it creates custom files with our own format .wzd, it also creates .meta files. 


### Camera ###

The Editor Camera in Wizard Engine provides a smooth and flexible way to explore your 3D scene.

We added frustrum culling and octree to our camera to enhance the performance in our engine.

We now can also create a camera gameobject that when you press the play button it becomes the POV in the viewport.

| Action | Control |
|--------|----------|
| Move | **W / A / S / D** while holding **Right Mouse Button (RMB)** |
| Move Faster | **Hold Shift** while moving |
| Look Around | **Hold Right Mouse Button (RMB)** and move the mouse |
| Orbit Around Object | **Hold Alt + Left Mouse Button (LMB)** |
| Zoom | **Mouse Scroll Wheel** |
| Center camera | ** press F to Center camera Into selected gameObject* |


This control scheme allows developers to quickly inspect models, position objects, and navigate large scenes seamlessly.

### Extra features ###
You can also duplicate and remove gameobject by right-clicking the gameobject.

We have keyboard shortcuts too: If you press crtl+S you save the current scene, If you press crtl+D you duplicate the scene, and if you press Supr you delete the current selected gameobject.

