#include "generator_model_loading.h"
#include <generator/SphereMesh.hpp>
#include <generator/BoxMesh.hpp>
#include <generator/PlaneMesh.hpp>
#include <list>

void LoadGeneratorModels(App* app)
{
	app->meshes.push_back(Mesh{});
	Mesh& mesh = app->meshes.back();
	u32 meshIdx = (u32)app->meshes.size() - 1u;

	app->models.push_back(Model{});
	Model& model = app->models.back();
	model.meshIdx = meshIdx;
	u32 modelIdx = (u32)app->models.size() - 1u;


	std::vector<float> vertices;
	std::vector<u32> indices;

	generator::SphereMesh sphere{};
	for (const generator::MeshVertex& vertex : sphere.vertices()) {
		
		//vertices.push_back(mesh->mVertices[i].x);
		//vertices.push_back(mesh->mVertices[i].y);
		//vertices.push_back(mesh->mVertices[i].z);
		//vertices.push_back(mesh->mNormals[i].x);
		//vertices.push_back(mesh->mNormals[i].y);
		//vertices.push_back(mesh->mNormals[i].z);

	}
}


