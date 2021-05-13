#include "model_loading.h"
#include "generator_model_loading.h"
#include <generator/SphereMesh.hpp>
#include <generator/BoxMesh.hpp>
#include <generator/PlaneMesh.hpp>


DefaultModel::DefaultModel(App* app)
{
	app->meshes.push_back(Mesh{});
	mesh = &app->meshes.back();
	meshIdx = (u32)app->meshes.size() - 1u;

	app->models.push_back(Model{});
	model = &app->models.back();
	model->meshIdx = meshIdx;
	modelIdx = (u32)app->models.size() - 1u;

	model->materialIdx.push_back(0u);
}


DefaultModel LoadSphereModel(App* app)
{
	DefaultModel modelToload(app);
	SphereMesh sphere{};

	for (const generator::MeshVertex& vertex : sphere.vertices()) {
		modelToload.vertices.push_back(vertex);
	}
	for (const generator::Triangle& triangle : sphere.triangles()) {
		modelToload.triangles.push_back(triangle);
	}
	return modelToload;
}

DefaultModel LoadBoxModel(App* app)
{
	DefaultModel modelToload(app);
	BoxMesh box{};

	for (const generator::MeshVertex& vertex : box.vertices()) {
		modelToload.vertices.push_back(vertex);
	}
	for (const generator::Triangle& triangle : box.triangles()) {
		modelToload.triangles.push_back(triangle);
	}
	return modelToload;
}

DefaultModel LoadPlaneModel(App* app)
{
	DefaultModel modelToload(app);
	PlaneMesh plane{};

	for (const generator::MeshVertex& vertex : plane.vertices()) {
		modelToload.vertices.push_back(vertex);
	}
	for (const generator::Triangle& triangle : plane.triangles()) {
		modelToload.triangles.push_back(triangle);
	}
	return modelToload;
}


void LoadGeneratorModels(App* app)
{
	std::list<DefaultModel> modelsToLoad
	{
		LoadSphereModel(app),
		LoadBoxModel(app),
		LoadPlaneModel(app)
	};

	for (const DefaultModel& defaultModel : modelsToLoad)
	{
		std::vector<float> vertices;
		std::vector<u32> indices;

		for (const generator::MeshVertex& vertex : defaultModel.vertices)
		{
			// Process vertices
			vertices.push_back(vertex.position.x);
			vertices.push_back(vertex.position.y);
			vertices.push_back(vertex.position.z);

			vertices.push_back(vertex.normal.x);
			vertices.push_back(vertex.normal.y);
			vertices.push_back(vertex.normal.z);

			vertices.push_back(vertex.texCoord.x);
			vertices.push_back(vertex.texCoord.y);
		}

		// Process indices 
		for (const generator::Triangle& triangle : defaultModel.triangles)
		{
			indices.push_back(triangle.vertices.x);
			indices.push_back(triangle.vertices.y);
			indices.push_back(triangle.vertices.z);
		}

		// Create the vertex format
		VertexBufferLayout vertexBufferLayout = {};
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
		vertexBufferLayout.stride = 6 * sizeof(float);

		vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
		vertexBufferLayout.stride += 2 * sizeof(float);

		// Add the submesh into the mesh
		Submesh submesh = {};
		submesh.vertexBufferLayout = vertexBufferLayout;
		submesh.vertices.swap(vertices);
		submesh.indices.swap(indices);
		defaultModel.mesh->submeshes.push_back(submesh);

		LoadMeshGlBuffers(*defaultModel.mesh);
	}
}

