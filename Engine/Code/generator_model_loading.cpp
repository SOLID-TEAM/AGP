#include "model_loading.h"
#include "generator_model_loading.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"

u32 LoadDefaultModel(DefaultModelType type, App* app)
{

	app->meshes.push_back(Mesh{});
	Mesh& mesh = app->meshes.back();
	u32 meshIdx = (u32)app->meshes.size() - 1u;

	app->models.push_back(Model{});
	Model& model = app->models.back();
	model.meshIdx = meshIdx;
	u32 modelIdx = (u32)app->models.size() - 1u;

	model.materialIdx.push_back(app->defaultMaterialId);

	par_shapes_mesh* parMesh = GenerateDefaultModelData(type);
	
	if (!parMesh->normals)
		par_shapes_compute_normals(parMesh);

	std::vector<float> vertices;
	std::vector<u32> indices;

	for (int i = 0; i < parMesh->npoints; ++i)
	{
		int pointOffset = i * 3;

		// Process vertices
		vertices.push_back(parMesh->points[pointOffset]);
		vertices.push_back(parMesh->points[pointOffset + 1]);
		vertices.push_back(parMesh->points[pointOffset + 2]);


		vertices.push_back(parMesh->normals[pointOffset]);
		vertices.push_back(parMesh->normals[pointOffset + 1]);
		vertices.push_back(parMesh->normals[pointOffset + 2]);

		if (!parMesh->tcoords)
		{
			vertices.push_back(parMesh->normals[pointOffset]);
			vertices.push_back(parMesh->normals[pointOffset + 1]);
		}
		else
		{
			vertices.push_back(parMesh->tcoords[i * 2]);
			vertices.push_back(parMesh->tcoords[i * 2 + 1]);
		}

	}									 

	// Process indices 
	for (int i = 0; i < parMesh->ntriangles; ++i)
	{
		int pointOffset = i * 3;

		indices.push_back(parMesh->triangles[pointOffset]);
		indices.push_back(parMesh->triangles[pointOffset + 1]);
		indices.push_back(parMesh->triangles[pointOffset + 2]);
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
	mesh.submeshes.push_back(submesh);

	LoadMeshGlBuffers(mesh);
	par_shapes_free_mesh(parMesh);

	return modelIdx;
}


par_shapes_mesh* GenerateDefaultModelData(DefaultModelType type)
{
	par_shapes_mesh* mesh = nullptr;

	switch (type)
	{
	case DefaultModelType::Sphere: {
		mesh = par_shapes_create_subdivided_sphere(4);
		break; }
	case DefaultModelType::Cube: {
		mesh = par_shapes_create_cube();

		break; }
	case DefaultModelType::Plane: {
		mesh = par_shapes_create_plane(1, 1);
		par_shapes_translate(mesh, -0.5f, -0.5f, 0.f);

		break; }

	default:
		break;
	}

	return mesh;
}
