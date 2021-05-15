//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "assimp_model_loading.h"
#include "generator_model_loading.h"

using namespace glm;

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf_s(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    app->mode = Mode_TexturedQuad;

    // Fill opengl info once at init
    FillOpenGLInfo(app);

    // GL_KHR_debug callback for show opengl errors
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 2))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    // uniform buffers -------------------------------------------

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->cbuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // camera -----------------------------------------------------

    app->camera.position = { 5.0, 5.0, 10.0 };
    app->camera.target = { 0.0, 0.0, 0.0 };

    // create some lights -----------------------------------------
    // directional front from up
    Light light = {};
    light.color = { 1.0, 1.0, 1.0 };
    light.direction = { 0, -1, -1 };
    light.type = LightType::LightType_Directional;
    app->lights.push_back(light);
    // directional back from up
    light.color = { 1.0, 1.0, 1.0 };
    light.direction = { 0, -1, 1 };
    app->lights.push_back(light);

    // point lights
    light.color = { 0.0, 0.0, 1.0 };
    light.position = { 0.0, 0.0, 6.0 };
    light.type = LightType::LightType_Point;
    app->lights.push_back(light);

    light.color = { 1.0, 0.0, 0.0 };
    light.position = { -6.0, 0.0, 3.0 };
    app->lights.push_back(light);

    light.color = { 0.0, 1.0, 0.0 };
    light.position = { 6.0, 0.0, 3.0 };
    app->lights.push_back(light);
    // ------------------------------------------------------------

    // Geometry
    //glGenBuffers(1, &app->embeddedVertices);
    //glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    //glGenBuffers(1, &app->embeddedElements);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //// Attribute state
    //glGenVertexArrays(1, &app->vao);
    //glBindVertexArray(app->vao);
    //glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0 );
    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12 );
    //glEnableVertexAttribArray(1);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    //glBindVertexArray(0);

    //// program initialization
    //app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    //Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    //app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    //// Texture initialization
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    //app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    //app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    //app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    // load program -------------------------------------------------

    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SIMPLE_PATRICK");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    FillInputVertexShaderLayout(texturedMeshProgram);

    // patrick ------------------------------------------------------
    vec3 patrickPositions[] = { {  0.0, 0.0, 0.0 }, 
                                {  6.0, 0.0, 0.0 },
                                { -6.0, 0.0, 0.0 } };
    Entity patrick = {};
    patrick.modelIndex = LoadModel(app, "Patrick/Patrick.obj");
    for (int i = 0; i < ARRAY_COUNT(patrickPositions); ++i)
    {
        patrick.worldMatrix = TransformPositionScale(patrickPositions[i], vec3(1.0));
        app->entities.push_back(patrick);
    }
    
    // ---------------------------------------------------------------

    // textured quad to new structs ----------------------------------
    // textured geometry program
    /*app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeoProgram = app->programs[app->texturedGeometryProgramIdx];
    FillInputVertexShaderLayout(texturedGeoProgram);

    // meshes etc
    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;

    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = meshIdx;
    u32 modelIdx = (u32)app->models.size() - 1u;

    // basic material
    u32 baseMeshMaterialIndex = (u32)app->materials.size();
    app->materials.push_back(Material{});
    Material& material = app->materials.back();
    material.name = "basic_white_mat";
    material.albedo = vec3(1.0f, 1.0f, 1.0f);
    //material.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
    //material.smoothness = 10 / 256.0f;
    material.albedoTextureIdx = app->diceTexIdx;
    model.materialIdx.push_back(baseMeshMaterialIndex);

    // create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 3 * sizeof(float) });
    vertexBufferLayout.stride = 5 * sizeof(float);

    // add submesh into mesh
    std::vector<float> v;
    std::vector<u32> ind;

    u32 n_vert = ARRAY_COUNT(vertices);
    for (u32 i = 0; i < n_vert; ++i)
    {
        v.push_back(vertices[i].pos.x);
        v.push_back(vertices[i].pos.y);
        v.push_back(vertices[i].pos.z);

        v.push_back(vertices[i].uv.x);
        v.push_back(vertices[i].uv.y);
    }

    u32 n_indices = ARRAY_COUNT(indices);
    for (u32 i = 0; i < n_indices; ++i)
    {
        ind.push_back(indices[i]);
    }

    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(v);
    submesh.indices.swap(ind);
    mesh.submeshes.push_back(submesh);

    // prelink vao to this submesh || TODO: this is the way?
    //FindVAO(mesh, 0, texturedGeoProgram);

   // Geometry
   glGenBuffers(1, &mesh.vertexBufferHandle);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &mesh.indexBufferHandle);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); */

   // ------------------------------------------------------------------------

   // Load Default Material 

   app->defaultMaterialId = (u32)app->materials.size();
   app->materials.push_back(Material{});

   Material& defaultMaterial = app->materials.back();
   defaultMaterial.name = "defaultMaterial";
   defaultMaterial.albedo = vec3(1.0f, 1.0f, 1.0f);
   defaultMaterial.albedoTextureIdx = app->whiteTexIdx;

   // Load Default Models

   //LoadDefaultModel(DefaultModelType::Sphere, app);

   for (int i = 0; i < (int)DefaultModelType::Max; ++i)
   {
       app->defaultModelsId[i] = LoadDefaultModel( (DefaultModelType)i, app);
   }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::End();

    if (app->showGlInfo)
    {
        ImGui::Begin("OpenGL info", &app->showGlInfo);
        ImGui::Text(app->glinfo.c_str());
        ImGui::End();
    }
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

    // show/hide gl info window
    if (app->input.keys[Key::K_I] == BUTTON_PRESS)
    {
        app->showGlInfo = !app->showGlInfo;
    }

    // shader hot reload check
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 lastTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (lastTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = lastTimestamp;
        }
    }

    // update projection and view mat
    UpdateProjectionView(app);

    // update uniform global/local params buffer block
    {
        BindBuffer(app->cbuffer);
        MapBuffer(app->cbuffer, GL_WRITE_ONLY);

        {
            app->globalParamsOffset = app->cbuffer.head;

            PushVec3(app->cbuffer, app->camera.position);
            PushUInt(app->cbuffer, app->lights.size());

            for (u32 i = 0; i < app->lights.size(); ++i)
            {
                AlignHead(app->cbuffer, sizeof(vec4));

                Light& l = app->lights[i];
                PushUInt(app->cbuffer, l.type);
                PushVec3(app->cbuffer, l.color);
                PushVec3(app->cbuffer, l.direction);
                PushVec3(app->cbuffer, l.position);
            }

            app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;
        }

        // update uniform local params buffer block
        {
            std::vector<Entity>& entities = app->entities;

            for (int i = 0; i < entities.size(); ++i)
            {
                // update entity transforms ------
                Entity& e = entities[i];
                mat4 worldViewProjection = app->projection * app->view * e.worldMatrix;

                // -------------------------------
                AlignHead(app->cbuffer, app->uniformBlockAlignment);
                e.localParamsOffset = app->cbuffer.head;

                PushMat4(app->cbuffer, e.worldMatrix);
                PushMat4(app->cbuffer, worldViewProjection);

                e.localParamsSize = app->cbuffer.head - e.localParamsOffset;
            }
        }

        UnmapBuffer(app->cbuffer);
    }
}



void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!


                /*glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
                glUseProgram(programTexturedGeometry.handle);
                glBindVertexArray(app->vao);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUniform1i(app->programUniformTexture, 0);
                glActiveTexture(GL_TEXTURE0);
                GLuint textureHandle = app->textures[app->diceTexIdx].handle;
                glBindTexture(GL_TEXTURE_2D , textureHandle);

                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

                glBindVertexArray(0);
                glUseProgram(0);*/

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                glEnable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

                for (int idx = 0; idx < app->entities.size(); ++idx)
                {
                    Entity& entity = app->entities[idx];
                    Model& model = app->models[entity.modelIndex];
                    Mesh& mesh = app->meshes[model.meshIdx];

                    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                    {
                        GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                        glBindVertexArray(vao);

                        u32 submeshMaterilIdx = model.materialIdx[i];
                        Material& submeshMaterial = app->materials[submeshMaterilIdx];

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                        //glUniform1i(app->texturedMeshProgram_uTexture, 0);

                        Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }
                }

                glBindVertexArray(0);
                glUseProgram(0);
            }
            break;

        default:;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle; // if we found a existing vao return it
    }

    GLuint vaoHandle = 0;
    // if no vao found, create a new and return it
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        // link all vertex inputs attributes to attributes in the vertex buffer
        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
        {
            bool attributeWasLinked = false;
            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
            {
                if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
                {
                    const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                    const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                    const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
                    const u32 stride = submesh.vertexBufferLayout.stride;
                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }
            assert(attributeWasLinked); // submesh must provide an attribute for each vertex input
        }
        glBindVertexArray(0);
    }

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;

}

void FillInputVertexShaderLayout(Program& program)
{
    GLint attributeCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (int i = 0; i < attributeCount; ++i)
    {
        GLchar attributeName[256];
        GLsizei attributeNameLength;
        GLint attributeSize;
        GLenum attributeType;

        glGetActiveAttrib(program.handle, i,
                          ARRAY_COUNT(attributeName),
                          &attributeNameLength,
                          &attributeSize,
                          &attributeType,
                          attributeName);
        
        GLint attributeLocation = glGetAttribLocation(program.handle, attributeName);

        program.vertexInputLayout.attributes.push_back({(u8)attributeLocation, (u8)attributeSize});
    }
}

void FillOpenGLInfo(App* app)
{
    std::string* s = &app->glinfo;
   
    s->clear();
    s->append("OpenGL version: "  + std::string((char*)glGetString(GL_VERSION))  + "\n" );
    s->append("OpenGL renderer: " + std::string((char*)glGetString(GL_RENDERER)) + "\n");
    s->append("OpenGL vendor: "   + std::string((char*)glGetString(GL_VENDOR))   + "\n");
    s->append("OpenGL GLSL version: " + std::string((char*)glGetString(GL_SHADING_LANGUAGE_VERSION)) + "\n");

    s->append("OpenGL extensions:\n");
    GLint n_ext;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n_ext);
    for (int i = 0; i < n_ext; ++i)
    {
        s->append((char*)glGetStringi(GL_EXTENSIONS, GLuint(i)) + std::string("\n"));
    }
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message: %s", msg);

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             ELOG(" - source: GL_DEBUG_SOURCE_API"); break; 
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break;
    case GL_DEBUG_SOURCE_APPLICATION:     ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break;
    case GL_DEBUG_SOURCE_OTHER:           ELOG(" - source: GL_DEBUG_SOURCE_OTHER, gl"); break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;
    case GL_DEBUG_TYPE_PORTABILITY:         ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break;
    case GL_DEBUG_TYPE_PERFORMANCE:         ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break;
    case GL_DEBUG_TYPE_MARKER:              ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break;
    case GL_DEBUG_TYPE_POP_GROUP:           ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break;
    case GL_DEBUG_TYPE_OTHER:               ELOG(" - type: GL_DEBUG_TYPE_OTHER, gl"); break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break;
    case GL_DEBUG_SEVERITY_MEDIUM: ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break;
    case GL_DEBUG_SEVERITY_LOW: ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break;
    }
}

void UpdateProjectionView(App* app)
{
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    float znear = 0.1f;
    float zfar = 1000.0f;
    app->projection = perspective(radians(60.0f), aspectRatio, znear, zfar);
    app->view = lookAt(app->camera.position, app->camera.target, { 0, 1, 0 });
}

mat4 TransformScale(const vec3& scaleFactors)
{
    mat4 transform = scale(scaleFactors);
    return transform;
}

mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors)
{
    mat4 transform = translate(pos);
    transform = scale(transform, scaleFactors);
    return transform;
}