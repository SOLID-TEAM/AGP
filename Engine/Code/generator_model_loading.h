#pragma once
#include <generator/MeshVertex.hpp>
#include <generator/Triangle.hpp>
#include "engine.h"
#include <list>

using namespace generator;

struct DefaultModel
{
	Mesh* mesh;
	Model* model;
	u32 modelIdx;
	u32 meshIdx;
	std::list<MeshVertex> vertices;
	std::list<Triangle> triangles;

	DefaultModel(App* app);
};

void LoadGeneratorModels(App* app);
