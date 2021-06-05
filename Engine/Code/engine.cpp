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

    // SSAO ------------------------------------------------------
    // SSAO kernel
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );

        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = Lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        app->ssaoKernel.push_back(sample);
    }

    // SSAO noise
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; ++i)
    {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &app->noiseTexture);
    glBindTexture(GL_TEXTURE_2D, app->noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // SSAO Framebuffer
    glGenFramebuffers(1, &app->ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoFBO);

    glGenTextures(1, &app->ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, app->ssaoColorBuffer);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, app->displaySize.x, app->displaySize.y, 0, GL_RED, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->ssaoColorBuffer, 0);

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        ELOG("error creating ssao framebuffer");
    }

    // SSAO Blur Framebuffer
    glGenFramebuffers(1, &app->ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoBlurFBO);
    glGenTextures(1, &app->ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, app->ssaoColorBufferBlur);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, app->displaySize.x, app->displaySize.y, 0, GL_RED, GL_FLOAT, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, app->ssaoColorBufferBlur,0);

    fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        ELOG("error creating ssao blur framebuffer");
    }

    // ------------------------------------------------------------
    

    // GL_KHR_debug callback for show opengl errors
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 2))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    // uniform buffers --------------------------------------------

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->cbuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Framebuffer object and textures attachments ----------------

    // position color buffer
    glGenTextures(1, &app->gPosition);
    glBindTexture(GL_TEXTURE_2D, app->gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // normal color buffer
    glGenTextures(1, &app->gNormal);
    glBindTexture(GL_TEXTURE_2D, app->gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // color + specular color buffer
    glGenTextures(1, &app->gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, app->gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // grayScale depth for combobox purposes
    glGenTextures(1, &app->gDepthGray);
    glBindTexture(GL_TEXTURE_2D, app->gDepthGray);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // depth buffer
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->gPosition, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->gNormal, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->gAlbedoSpec, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->gDepthGray, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->gFinalPass, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  app->depthAttachmentHandle, 0);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (framebufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                     ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:        ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:        ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                   ELOG("GL_FRAMEBUFFER_UNSUPPORTED");
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:        ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:      ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
        }
    }

    /*GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);*/
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
   

    // final buffer to blit the resulted lighting texture
    // final lighted and shaded scene texture
    glGenTextures(1, &app->gFinalPass);
    glBindTexture(GL_TEXTURE_2D, app->gFinalPass);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // depth buffer
    glGenTextures(1, &app->finalPassDepth);
    glBindTexture(GL_TEXTURE_2D, app->finalPassDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->finalPassBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->finalPassBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->gFinalPass, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->finalPassDepth, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        ELOG("error creating final framebuffer pass");
    }

    app->selectedAttachment = app->gFinalPass;

    // z pre pass fbo -------------
    glGenTextures(1, &app->zPrePassDepth);
    glBindTexture(GL_TEXTURE_2D, app->zPrePassDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->zPrePassFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, app->zPrePassFbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->zPrePassDepth, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        ELOG("error creating zprepass framebuffer object");
    }
   // -----------------------------------

    // camera -----------------------------------------------------

    app->camera.position = { -3.5f , .0f , -22.f};
    app->camera.finalPitch = app->camera.finalYaw = app->camera.pitch = app->camera.yaw = 0.f;

    app->camera.finalPitch = 25;
    app->camera.finalYaw = -0.22;
    app->camera.pitch = 25;
    app->camera.yaw = -0.22;

    // ------------------------------------------------------------

    //// Texture initialization
    //app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png"); // TODO: fix texture location
    //app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    //app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    //app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    // load z pre pass program --------------------------------------
    app->zPrePassProgramIdx = LoadProgram(app, "shaders.glsl", "Z_PRE_PASS");
    Program& zPreProg = app->programs[app->zPrePassProgramIdx];
    FillInputVertexShaderLayout(zPreProg);

    // load SSAO program --------------------------------------------
    app->ssaoProgramIdx = LoadProgram(app, "shaders.glsl", "SSAO_PASS");
    Program& ssaoProg = app->programs[app->ssaoProgramIdx];
    FillInputVertexShaderLayout(ssaoProg);

    // load SSAO blur program ---------------------------------------
    app->ssaoBlurProgramIdx = LoadProgram(app, "shaders.glsl", "SSAO_BLUR_PASS");
    Program& ssaoBlurProg = app->programs[app->ssaoBlurProgramIdx];
    FillInputVertexShaderLayout(ssaoBlurProg);

    // load geometry first pass program -----------------------------
    app->geometryPassProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_PASS");
    Program& p = app->programs[app->geometryPassProgramIdx];
    FillInputVertexShaderLayout(p);

    // load lighting second pass program ----------------------------
    // independent program for directionals and point lights
    app->dirLightPassProgramIdx = LoadProgram(app, "shaders.glsl", "LIGHT_PASS_VOLUMES");
    Program& dirLightPassProgram = app->programs[app->dirLightPassProgramIdx];
    FillInputVertexShaderLayout(dirLightPassProgram);

    // ---------------------------------------------------------------

    // textured quad to new structs ----------------------------------
    // textured geometry program
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeoProgram = app->programs[app->texturedGeometryProgramIdx];
    FillInputVertexShaderLayout(texturedGeoProgram);

    // meshes etc
    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;
    app->texturedQuadMeshIdx = meshIdx;

    // create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 2, 3 * sizeof(float) });
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
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

   // ------------------------------------------------------------------------

   // Load Default Material 

   app->defaultMaterialId = (u32)app->materials.size();
   app->materials.push_back(Material{});

   Material& defaultMaterial = app->materials.back();
   defaultMaterial.name = "defaultMaterial";
   defaultMaterial.albedo = vec3(.8f, .8f, .8f);
   defaultMaterial.albedoTextureIdx = app->whiteTexIdx;

   // Load Default Models

   app->defaultModelsId[(int)DefaultModelType::Sphere] = LoadDefaultModel(DefaultModelType::Sphere, app);
   app->defaultModelsId[(int)DefaultModelType::Plane] = LoadModel(app, "Plane/plane.obj");
   app->defaultModelsId[(int)DefaultModelType::Cube] = LoadModel(app, "Cube/cube.obj");
   app->defaultModelsId[(int)DefaultModelType::Torus] = LoadModel(app, "Torus/torus.obj");
   app->defaultModelsId[(int)DefaultModelType::Cone] = LoadModel(app, "Cone/cone.obj");
   app->defaultModelsId[(int)DefaultModelType::Suzanne] = LoadModel(app, "Suzanne/suzanne.obj");

   // Default Scene

   // Models
   {
       Entity defaultElement = {};

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Plane];

       defaultElement.worldMatrix = TransformWorldMatrix({ 0.f,-3.8f,0.f }, { 0,0,0 }, vec3(30.0));
       app->entities.push_back(defaultElement);

       defaultElement.worldMatrix = TransformWorldMatrix({- 10.f,-3.8f,3.f }, { 0, 0,-90 }, vec3(1, 2, 1));
       app->entities.push_back(defaultElement);

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Cone];

       defaultElement.worldMatrix = TransformWorldMatrix(vec3(7.f, 4.f, 2.f), vec3(-30.f, 0.f, 30.f), vec3(2.f));
       app->entities.push_back(defaultElement);

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Torus];

       defaultElement.worldMatrix = TransformWorldMatrix({ 0.f,3, 5.f }, { 0,0,0 }, vec3(1.f));
       app->entities.push_back(defaultElement);

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Cube];

       defaultElement.worldMatrix = TransformWorldMatrix({ 0.f,3.8,-5.f }, { 30,-30,30 }, vec3(2.f));
       app->entities.push_back(defaultElement);

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Sphere];

       defaultElement.worldMatrix = TransformWorldMatrix({ -7.f,0.5 ,7.f }, { 30,-30,30 }, vec3(2.f));
       app->entities.push_back(defaultElement);

       defaultElement.modelIndex = app->defaultModelsId[(int)DefaultModelType::Suzanne];

       defaultElement.worldMatrix = TransformWorldMatrix({ -5.f,3.8, 10.f }, { 30,-30,30 }, vec3(2.f));
       app->entities.push_back(defaultElement);


       Entity patrick = {};
       patrick.modelIndex = LoadModel(app, "Patrick/Patrick.obj");

       patrick.worldMatrix = TransformWorldMatrix(vec3(0, 0., 5.), vec3(0.f), vec3(1.0));
       app->entities.push_back(patrick);

       patrick.worldMatrix = TransformWorldMatrix(vec3(6, 0., 10.), vec3(0.f, 30, 0), vec3(1.0));
       app->entities.push_back(patrick);

       patrick.worldMatrix = TransformWorldMatrix(vec3(-6, 0. ,0.), vec3(30.f, -180, 0), vec3(1.0));
       app->entities.push_back(patrick);


       patrick.worldMatrix = TransformWorldMatrix(vec3(6, 0., 0.), vec3(30.f, -60, 0), vec3(0.5));
       app->entities.push_back(patrick);


   }
   // Lights
   {
       vec3 colorRed =    vec3(1.0, 0.0, 0.0 );
       vec3 colorGreen =  vec3(0.0, 1.0, 0.0 );
       vec3 colorBlue =   vec3(0.0, 0.0, 1.0 );
       vec3 colorYellow = vec3(1.0, 1.0, 0.0 ) * 0.5f;
       vec3 colorWhite =  vec3(1.0, 1.0, 1.0 );
       vec3 colorPurple =  vec3(1.0, 0.0, 1.0 ) * 0.5f;
       vec3 colorOrange =  vec3(1.0, 0.6, 0.0 ) * 0.5f;

       Light light = {};

       // Directional Lights
       light.type = LightType::LightType_Directional;
       light.color = vec3(0.6);
       light.direction = { 0, -1, -2 };
       app->lights.push_back(light);

       // point lights

       light.type = LightType::LightType_Point;

       light.color = colorBlue;
       light.position = { 0.0, 3.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorRed;
       light.position = { -6.0, 3.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorGreen;
       light.position = { 6.0, 3.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorYellow ;
       light.position = { 6.0, 3.0, -10.0 };
       app->lights.push_back(light);

       light.color = colorPurple;
       light.position = { 0.0, 3.0, -10.0 };
       app->lights.push_back(light);

       light.color = colorOrange;
       light.position = { -6.0, 3.0, -10.0 };
       app->lights.push_back(light);


       light.color = colorBlue;
       light.position = { 0.0, -6.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorRed;
       light.position = { -6.0,-6.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorGreen;
       light.position = { 6.0, -6.0, -2.0 };
       app->lights.push_back(light);

       light.color = colorYellow;
       light.position = { 6.0, -6.0, -10.0 };
       app->lights.push_back(light);

       light.color = colorPurple;
       light.position = { 0.0,-6.0, -10.0 };
       app->lights.push_back(light);

       light.color = colorOrange;
       light.position = { -6.0, -6.0, -10.0 };
       app->lights.push_back(light);
   }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    const char* items[] = { "position", "normals", "albedo", "depth", "depth_grayscale", "final pass", "etc"};
    u32 attachments[] = { app->gPosition, app->gNormal, app->gAlbedoSpec, app->depthAttachmentHandle ,app->gDepthGray, app->gFinalPass};
    static const char* item_current = items[5];            // Here our selection is a single pointer stored outside the object.
    if (ImGui::BeginCombo("Texture", item_current)) // The second parameter is the label previewed before opening the combo.
    {
        for (int n = 0; n < IM_ARRAYSIZE(attachments); n++)
        {
            bool is_selected = (item_current == items[n]);
            if (ImGui::Selectable(items[n], is_selected))
            { 
                item_current = items[n];
                app->selectedAttachment = attachments[n];
            }
                
            /*if (is_selected)
                ImGui::SetItemDefaultFocus();  */ // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
        }
        ImGui::EndCombo();
    }
    ImGui::End();

    if (app->showGlInfo)
    {
        ImGui::Begin("OpenGL info", &app->showGlInfo);
        ImGui::Text(app->glinfo.c_str());
        ImGui::End();
    }

    ImGui::Begin("RenderTest");
    ImGui::Image((ImTextureID)app->ssaoColorBufferBlur, { (float)app->displaySize.x, (float)app->displaySize.y }, { 0,1 }, { 1,0 });
    ImGui::End();
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

    UpdateCamera(app);
    UpdateProjectionView(app);

    // update uniform global/local params buffer block
    {
        BindBuffer(app->cbuffer);
        MapBuffer(app->cbuffer, GL_WRITE_ONLY);

        {
            app->globalParamsOffset = app->cbuffer.head;

            PushVec3(app->cbuffer, -app->camera.position);
            PushUInt(app->cbuffer, app->lights.size());

            for (u32 i = 0; i < app->lights.size(); ++i)
            {
                AlignHead(app->cbuffer, sizeof(vec4));

                Light& l = app->lights[i];
                PushUInt(app->cbuffer, l.type);
                PushVec3(app->cbuffer, l.color);
                PushVec3(app->cbuffer, l.direction);
                PushVec3(app->cbuffer,- l.position);
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
                }

                // Z Pre pass for both rendering pipelines forward/deferred
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, app->zPrePassFbo);

                    glDepthMask(GL_TRUE);
                    //glColorMask(0, 0, 0, 0);
                    glDepthFunc(GL_LESS);

                    glEnable(GL_DEPTH_TEST);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    //glClearColor(0, 0, 0, 1);
                    glColorMask(0, 0, 0, 0);

                    Program& prePassProg = app->programs[app->zPrePassProgramIdx]; //
                    glUseProgram(prePassProg.handle);

                    // render scene entities -----
                    for (int idx = 0; idx < app->entities.size(); ++idx)
                    {
                        Entity& entity = app->entities[idx];
                        Model& model = app->models[entity.modelIndex];
                        Mesh& mesh = app->meshes[model.meshIdx];

                        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                        {
                            Submesh& submesh = mesh.submeshes[i];
                            GLuint vao = FindVAO(mesh, i, prePassProg);
                            glBindVertexArray(vao);
                          
                            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                        }
                    }

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    // ---------------------------

                    /*glDepthMask(GL_FALSE);
                    glColorMask(1, 1, 1, 1);
                    glDepthFunc(GL_EQUAL);*/

                    //glBindVertexArray(0);
                }

                // Geometry pass -------------------------------------------------------------------

                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glColorMask(1, 1, 1, 1);
                glDepthFunc(GL_EQUAL);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // bind default zbuffer for read (from z pre pass depth)
                glBindFramebuffer(GL_READ_FRAMEBUFFER, app->zPrePassFbo);
                // bind gbuffer to write
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->gBuffer);

                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // copy default zbuffer depth to gbuffer fbo depth
                GLint w, h;
                w = app->displaySize.x;
                h = app->displaySize.y;
                glBlitFramebuffer(0,0, w,h, 0,0,w,h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

                GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
                glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

                Program& texturedMeshProgram = app->programs[app->geometryPassProgramIdx/*app->texturedMeshProgramIdx*/];
                glUseProgram(texturedMeshProgram.handle);

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

                for (int idx = 0; idx < app->entities.size(); ++idx)
                {
                    Entity& entity = app->entities[idx];
                    Model& model = app->models[entity.modelIndex];
                    Mesh& mesh = app->meshes[model.meshIdx];

                    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                    {
                        Submesh& submesh = mesh.submeshes[i];
                        
                        GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                        glBindVertexArray(vao);

                        u32 submeshMaterilIdx = model.materialIdx[i];
                        Material& submeshMaterial = app->materials[submeshMaterilIdx];

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                        //glUniform1i(app->texturedMeshProgram_uTexture, 0);

                        //Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }
                }

                glBindVertexArray(0);
                glUseProgram(0);

                //glBindFramebuffer(GL_FRAMEBUFFER, 0);
                /*glDepthMask(GL_FALSE);
                glDisable(GL_DEPTH_TEST);*/
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // SSAO PASS -------------------------------------------------------------
                {
                   /*glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    glColorMask(1, 1, 1, 1);
                    glDepthFunc(GL_LESS);*/

                    glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoFBO);
                    glClear(GL_COLOR_BUFFER_BIT);

                    // use ssao program
                    Program& ssaoProg = app->programs[app->ssaoProgramIdx];
                    glUseProgram(ssaoProg.handle);
                    
                    // bind sampler textures
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->gPosition);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, app->gNormal);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, app->noiseTexture);    

                    // send kernel samples
                    glUniform3fv(glGetUniformLocation(ssaoProg.handle, "samples"), 64, &app->ssaoKernel[0][0]);
                    // send projection and view matrix
                    glUniformMatrix4fv(glGetUniformLocation(ssaoProg.handle, "projection"), 1, GL_FALSE, &app->projection[0][0]);
                    glUniformMatrix4fv(glGetUniformLocation(ssaoProg.handle, "view"), 1, GL_FALSE, &app->view[0][0]);
                    
                    // render screen quad
                    RenderScreenQuad(app->ssaoProgramIdx, app);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glUseProgram(0);
                }
                
                // SSAO Blur pass ------
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, app->ssaoBlurFBO);
                    glClear(GL_COLOR_BUFFER_BIT);

                    Program& ssaoBlurProg = app->programs[app->ssaoBlurProgramIdx];
                    glUseProgram(ssaoBlurProg.handle);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->ssaoColorBuffer);

                    RenderScreenQuad(app->ssaoBlurProgramIdx, app);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glUseProgram(0);
                }

                // -----------------------------------------------------------------------

                // lighting pass ---------------------------------------------------------
                {
                    glDepthMask(GL_FALSE);
                    glDisable(GL_DEPTH_TEST);

                    glBindFramebuffer(GL_FRAMEBUFFER, app->finalPassBuffer);

                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_ONE, GL_ONE);

                    glClear(GL_COLOR_BUFFER_BIT);

                    // ----------
                    
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->gPosition);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, app->gNormal);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, app->gAlbedoSpec);

                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, app->ssaoColorBufferBlur);

                    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

                    // NOTE: 4.2 > glsl -> layout(binding = x) uniform sampler2D texName
                    //// setting uniforms sampler locations

                    Program& prog = app->programs[app->dirLightPassProgramIdx];
                    glUseProgram(prog.handle);

                    GLuint lightIdxLocation = glGetUniformLocation(prog.handle, "lightIdx");
                    GLuint worldViewProjectionLocation = glGetUniformLocation(prog.handle, "WVP");
                   

                    for (int i = 0; i < app->lights.size(); ++i)
                    {

                        Light& l = app->lights[i];
                       

                        if (l.type == LightType::LightType_Directional)
                        {
                            mat4 MVP = mat4(1.0);
                            glUniform1i(lightIdxLocation, i);
                            glUniformMatrix4fv(worldViewProjectionLocation, 1, GL_FALSE, &MVP[0][0]);

                            RenderScreenQuad(app->dirLightPassProgramIdx, app);

                        }
                        else
                        {
                            // this values must match with shader calculations
                            // for now, we have all light points attenuation values hardcoded on shader with this below values
                            float constant = 1.0;
                            float linear = 0.09;
                            float quadratic = 0.032;
                            float lightMax = std::fmaxf(std::fmaxf(l.color.r, l.color.g), l.color.b);
                            float radius = (-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax)))
                                / (2 * quadratic);

                            glEnable(GL_CULL_FACE); // render light effect only once
                            glCullFace(GL_FRONT);   // render the light volume if the camera is inside the sphere volume too 
                            mat4 pWorldMatrix = TransformPositionScale(-l.position, vec3(radius));
                            mat4 MVP = app->projection * app->view * pWorldMatrix;
                            glUniform1i(lightIdxLocation, i);
                            glUniformMatrix4fv(worldViewProjectionLocation, 1, GL_FALSE, &MVP[0][0]);

                            // render sphere with scale fitting the light volume radius
               
                            Model& model = app->models[app->defaultModelsId[(int)DefaultModelType::Sphere]];
                            Mesh& mesh = app->meshes[model.meshIdx];

                            //glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, s.localParamsOffset, s.localParamsSize);
                                                        
                            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                            {
                                GLuint vao = FindVAO(mesh, i, prog);
                                glBindVertexArray(vao);

                                Submesh& submesh = mesh.submeshes[i];
                                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                            }

                            glDisable(GL_CULL_FACE);

                        }
                    }

                    
                    glBindVertexArray(0);
                    glUseProgram(0);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                   
                }

                // render screen quad with selected texture from combobox
                {
                  
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    Program& texGeoProgram = app->programs[app->texturedGeometryProgramIdx];
                    glUseProgram(texGeoProgram.handle);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->selectedAttachment);

                    RenderScreenQuad(app->texturedGeometryProgramIdx, app);

                    glUseProgram(0);
                }

                
            }

            break;

        default:;
    }
}

void RenderScreenQuad(u32 programIdx, App* app)
{
    Mesh& mesh = app->meshes[app->texturedQuadMeshIdx];

    Program& p = app->programs[programIdx];
    //glUseProgram(p.handle);

    GLuint vao = FindVAO(mesh, 0, p);
    glBindVertexArray(vao);

    /*glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

    //glUniform1i(app->programUniformTexture, 0);


    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
   // glUseProgram(0);
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

void UpdateCamera(App* app)
{
    vec3 finalPosition(0.f);
    float cameraSpeed = 8.f;
    bool doubleSpeed = false;

    // Orientation 

    if (app->input.mouseButtons[0] == BUTTON_PRESSED && app->input.mouseDelta != vec2(0.f))
    {
        app->camera.finalYaw += app->input.mouseDelta.x * 0.005f ;
        app->camera.finalPitch += app->input.mouseDelta.y * 0.005f;
    }
    
    app->camera.yaw = Lerp(app->camera.yaw, app->camera.finalYaw, clamp(app->deltaTime * 10.f, 0.f, 1.f));
    app->camera.pitch = Lerp( app->camera.pitch, app->camera.finalPitch, clamp(app->deltaTime * 10.f, 0.f, 1.f));

    quat rotYaw = glm::angleAxis(app->camera.yaw, vec3(0.f, 1.f, 0.f));
    quat rotPitch = glm::angleAxis(app->camera.pitch, vec3(1.f, 0.f, 0.f));
    app->view = glm::mat4_cast( rotPitch * rotYaw);

    vec3 right = vec3(app->view[0][0], app->view[1][0], app->view[2][0]);
    vec3 forward = vec3(app->view[0][2], app->view[1][2], app->view[2][2]);
    vec3 up = vec3(0.f, 1.f, 0.f);

    // Movement 

    if (app->input.keys[Key::K_D] == BUTTON_PRESSED)
    {
        finalPosition -= right;
    }
    if (app->input.keys[Key::K_A] == BUTTON_PRESSED)
    {
        finalPosition += right;
    }
    if (app->input.keys[Key::K_W] == BUTTON_PRESSED)
    {
        finalPosition += forward;
    }
    if (app->input.keys[Key::K_S] == BUTTON_PRESSED)
    {
        finalPosition -= forward;
    }
    if (app->input.keys[Key::K_Q] == BUTTON_PRESSED)
    {
        finalPosition += up;
    }
    if (app->input.keys[Key::K_E] == BUTTON_PRESSED)
    {
        finalPosition -= up;
    }
    if (app->input.keys[Key::K_C] == BUTTON_PRESSED)
    {
        doubleSpeed = true;
    }

    if (finalPosition != vec3(0.f))
    {
        app->camera.position += glm::normalize(finalPosition) * cameraSpeed * app->deltaTime * (doubleSpeed ? 2.f : 1.f);
    }

    app->view = glm::translate(app->view, app->camera.position);
}

void UpdateProjectionView(App* app)
{
    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    float znear = 0.1f;
    float zfar = 1000.0f;
    app->projection = perspective(radians(60.0f), aspectRatio, znear, zfar);
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

mat4 TransformWorldMatrix(const vec3& position, const vec3& rotation, const vec3& scaleFactors)
{
    mat4 transform = translate(position);
  
    transform = rotate(transform, glm::radians(rotation.x), vec3(1,0,0));
    transform = rotate(transform, glm::radians(rotation.y), vec3(0,1,0));
    transform = rotate(transform, glm::radians(rotation.z), vec3(0,0,1));
    transform = scale(transform, scaleFactors);
    return transform;

}

float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}
