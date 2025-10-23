#pragma once  
#include "Module.h"  
#include <SDL3/SDL.h>  
#include <vector>  
struct SDL_Window;

enum class ShapeType { Cube, Sphere, Cylinder, Pyramid };
static ShapeType currentShape = ShapeType::Cube;

class OpenGL : public Module
{
public:
	OpenGL();
	~OpenGL();

	SDL_GLContext glContext;
	unsigned int shaderProgram;
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;

private:
	bool Start() override;
	bool CleanUp() override;
	bool Update() override;

	unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
	unsigned int sphereIndexCount = 0;
	unsigned int pyramidVAO = 0, pyramidVBO = 0, pyramidEBO = 0;
	unsigned int pyramidIndexCount = 0;
	unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
	unsigned int cylinderIndexCount = 0;

	void GenerateSphere(float radius, unsigned int stacks, unsigned int slices);
	void GeneratePyramid(float baseSize, float height);
	void GenerateCylinder(float radius, float height, int sectors);

};
