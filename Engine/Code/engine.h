//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>
#include "buffer_management.h"
#include <random>

#define BINDING(b) b

using namespace glm;

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;


enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    LightType    type;
    vec3         color;
    vec3         direction;
    vec3         position;
};

struct Entity
{
    mat4 worldMatrix;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
};

struct Camera
{
    vec3 position;
    //vec3 target;
    float yaw, pitch;
    float finalYaw, finalPitch;

};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

// Hardcoded plane model with uv coords and indices -----

const VertexV3V2 vertices[] = {
    { glm::vec3(-1.0, -1.0, 0.0), glm::vec2(0.0, 0.0)}, // bottom left vertex
    { glm::vec3(1.0, -1.0, 0.0), glm::vec2(1.0, 0.0) }, // bottom right vertex
    { glm::vec3(1.0, 1.0, 0.0), glm::vec2(1.0, 1.0)} , // top right vertex
    { glm::vec3(-1.0, 1.0, 0.0), glm::vec2(0.0, 1.0) } // top left vertex
};

const u32 indices[] = {
    0, 1, 2,
    0, 2, 3
};

// -------------------------------------------------------
// -------------------------------------------------------
// VBO, EBO, SHADER, VAO STUFF ---------------------------

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

// --------------------------------------------------
// MODELS, MESHES, MATERIALS -----------------------------

struct Model
{
    u32              meshIdx;
    std::vector<u32> materialIdx;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector<u32>   indices;
    u32                vertexOffset;
    u32                indexOffset;

    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
    GLuint               vertexBufferHandle;
    GLuint               indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp;
    VertexShaderLayout vertexInputLayout;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count
};

enum class DefaultModelType : int
{
    Sphere = 0,
    Cube,
    Plane,
    Torus,
    Cone,
    Suzanne,
    Max,
};

struct App
{
    // gl info: stores glinfo in one string to be requested by user input
    // then shows in one imgui window
    std::string glinfo;
    bool showGlInfo = false;

    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Material> materials;
    std::vector<Mesh>     meshes;
    std::vector<Model>    models;
    std::vector<Program>  programs;
    std::vector<Light>    lights;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    u32 geometryPassProgramIdx;
    u32 dirLightPassProgramIdx;
    //u32 pointLightPassProgramIdx;
    u32 zPrePassProgramIdx;
    u32 fordwardProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;
    // location on samplers from lighting pass program
    //GLuint gPosSampler;
    //GLuint gNormSampler;
    //GLuint gAlbeSampler;
    // default texture uniform for default model
    GLuint texturedMeshProgram_uTexture;
        
    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // Default definitions

    u32 defaultModelId = 0;
    u32 defaultModelsId[(int)DefaultModelType::Max];
    u32 defaultMaterialId;

    // mesh for textured quad
    u32 texturedQuadMeshIdx;

    // -----
    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    //GLuint bufferHandle;
    Buffer cbuffer;
    u32 globalParamsOffset;
    u32 globalParamsSize;

    // framebuffer object and attachments
    GLuint gBuffer;
    GLuint gPosition;
    GLuint gNormal;
    GLuint gAlbedoSpec;
    GLuint gDepthGray;
    GLuint depthAttachmentHandle;
    
    GLuint gFinalPass;
    GLuint finalPassBuffer;
    GLuint finalPassDepth;

    GLuint zPrePassFbo;
    GLuint zPrePassDepth;

    GLuint selectedAttachment; // imgui combobox

    //glm::mat4 worldMatrix;
    //glm::mat4 worldViewProjectionMatrix;
    mat4 view;
    mat4 projection;

    std::vector<Entity> entities;

    Camera camera;
    uint cubeMapId;
    u32 skyboxProgramIdx;
    u32 skyboxMeshIdx;
    bool doFakeReflections = false;

    // SSAO
    u32 ssaoProgramIdx;
    u32 ssaoBlurProgramIdx;
    std::vector<vec3> ssaoKernel;
    GLuint ssaoFBO;
    GLuint ssaoBlurFBO;
    GLuint ssaoColorBuffer;
    GLuint ssaoColorBufferBlur;
    GLuint noiseTexture;
    bool viewSkybox = true;
    bool doSSAO = true;
    bool doSSAOBlur = true;

    // pipeline selection
    bool deferred = true;
};


void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);
void RenderScreenQuad(u32 programIdx, App* app);

//
u32 LoadTexture2D(App* app, const char* filepath);
uint LoadCubemap(std::vector<std::string> facesPaths);
//
void UpdateCamera(App* app);
void UpdateProjectionView(App* app);
mat4 TransformScale(const vec3& scaleFactors);
mat4 TransformPositionScale(const vec3& pos, const vec3& scaleFactors);
mat4 TransformWorldMatrix(const vec3& position, const vec3& rotation, const vec3& scaleFactors);
float Lerp(float a, float b, float f);
//
void FillOpenGLInfo(App* app);
void FillInputVertexShaderLayout(Program& program);
GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* userParam);

