#pragma once

#include "engine.h"

struct par_shapes_mesh_s;

u32 LoadDefaultModel(DefaultModelType type, App* app);
par_shapes_mesh_s* GenerateDefaultModelData(DefaultModelType type);