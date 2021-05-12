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

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
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
    //app->diceTexIdx = LoadTexture2D(app, "dice.png");
    //app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    //app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    //app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    //app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    // patrick ----------------------

    // create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, 3 * sizeof(float) });
    vertexBufferLayout.stride = 5 * sizeof(float);

    // load program
    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SIMPLE_PATRICK");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    FillInputVertexShaderLayout(texturedMeshProgram);

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
            }
            break;

        default:;
    }
}

void FillInputVertexShaderLayout(Program& program)
{
    GLint attributeCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    for (int i = 0; i < attributeCount; ++i)
    {
        GLchar* attributeName = nullptr;
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