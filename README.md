# Wizard Engine #
Hello and welcome to Wizard Engine, a new Game Engine that focuses on 3D environments.

https://github.com/AsiGamer29/WizardEngine

## About ##
Wizard Engine has been made by:
- Asier Ulloa: https://github.com/AsiGamer29
- Aniol López: https://github.com/Aniolobolo
- Saüc Pellejero: https://github.com/ZReiNa

We are students from CITM at UPC, and we created this engine for an assignment at our university.

## Engine characteristics ##
### Description ###
In this Game Engine, you can create Game Objects, add a 3D model to them, a texture and a material, and you can move it around, rotate it, and scale it.

### Dynamic Camera ###
We have also aded a dynamic camera that you can move at will pressing right click and moving with your WASD keys. Pressing shift also makes you move the camera faster.

You can also orbit around the mouse pressing the alt key and clicking your left mouse key and dragging your mouse around.

We implemented that after pressing the F key you can now center the camera on the selected gameObject.

We implemented that you can create a camera as a gameObject too, so you are not rooted to use de default one, you can move it as freely as you want, and when you press play the camera you've created becomes your POV.

We now also have frustrum culling and octree implemented in to our camera.

### User Interface ###
The user interface is quite simmilar to other Game Engines, in that you can select your file, open/close pop-up windows, and also click the Help button to go to our documentation, bug reports, or simply know who we are and who made this engine.

We implemented the Imguizmo library, it let's us transform and modify our gameobjects: Rotation,Scale and Position.

By interacting with the UI, you can create GameObjects using different shapes, such as spheres, pyramids, cubes, and more!

Now you can reparent and move the gameopjects from the hierarchy. You also can create empty game objects.

You can change the transparency of the textures and rotate the UVs from the editor.

We have implemented an asset manager: In the engine you can now see all the folders and files that are within the project itself.

Selecting GameObjects in the Hierarchy window lets you modify values such as texture, position, rotation and scale.

### Resource Manager ###
We have files with unique format(.wzd). They change depending on the source(if its a texture.wzt, if its a model .wzm etc...).

We have also added .meta files, which can be dragged and dropped in to the scene using the asset manager.

You can also save and load scenes into the engine.

### Extra Features ###
You can also duplicate and remove gameobject by right-clicking the gameobject.

We have keyboard shortcuts too: If you press crtl+S you save the current scene, If you press crtl+D you duplicate the scene, and if you press Supr you delete the current selected gameobject.


