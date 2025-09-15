#include "renderer.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "logger.h"

#ifdef USE_IMGUI
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#endif

// OpenGL constants
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30

// OpenGL function pointers
typedef void(APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void(APIENTRY* PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void(APIENTRY* PFNGLBUFFERDATAPROC)(GLenum target,
                                            ptrdiff_t size,
                                            const void* data,
                                            GLenum usage);
typedef void(APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void(APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void(APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void(APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index,
                                                     GLint size,
                                                     GLenum type,
                                                     GLboolean normalized,
                                                     GLsizei stride,
                                                     const void* pointer);
typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void(APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader,
                                              GLsizei count,
                                              const char* const* string,
                                              const GLint* length);
typedef void(APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void(APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void(APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void(APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const char* name);
typedef void(APIENTRY* PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void(APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat* value);
typedef void(APIENTRY* PFNGLBUFFERSUBDATAPROC)(GLenum target,
                                               ptrdiff_t offset,
                                               ptrdiff_t size,
                                               const void* data);
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM3FPROC glUniform3f;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
static PFNGLBUFFERSUBDATAPROC glBufferSubData;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

void Renderer::LoadOpenGLFunctions() {
    // Suppress GCC's cast-function-type warnings for WGL-proc casts in this function only
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glEnableVertexAttribArray =
        (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer =
        (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

bool Renderer::Initialize(HWND hwnd) {
    srand((unsigned int)time(nullptr));
    hdc = GetDC(hwnd);
    // Diagnose whether the ImGui code path is compiled in this TU
#ifdef USE_IMGUI
    LOG_INFO("[renderer.cpp] USE_IMGUI is defined");
#else
    LOG_WARN("[renderer.cpp] USE_IMGUI is NOT defined");
#endif

    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                                 PFD_TYPE_RGBA,
                                 32,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24,
                                 8,
                                 0,
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);

    LoadOpenGLFunctions();

    const char* gl_version = (const char*)glGetString(GL_VERSION);
    const char* gl_renderer = (const char*)glGetString(GL_RENDERER);
    const char* gl_vendor = (const char*)glGetString(GL_VENDOR);
    LOG_INFO("OpenGL Version: %s", gl_version ? gl_version : "<null>");
    LOG_INFO("OpenGL Renderer: %s", gl_renderer ? gl_renderer : "<null>");
    LOG_INFO("OpenGL Vendor: %s", gl_vendor ? gl_vendor : "<null>");

    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        uniform mat4 uView;
        uniform mat4 uProjection;
        uniform vec3 uColor;
        out vec3 vertexColor;
        void main() {
            gl_Position = uProjection * uView * vec4(aPos, 1.0);
            vertexColor = aColor * uColor;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vertexColor, 1.0);
        }
    )";

    shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);
    CreateGrid();
    CreateCube();
    CreateCrosshair();

    glEnable(GL_DEPTH_TEST);

    // Enable VSync to prevent tearing and reduce flickering
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }

#ifdef USE_IMGUI
    LOG_INFO("Initializing ImGui...");
    InitImGui(hwnd);
    LOG_INFO("ImGui initialized.");
#endif

    return true;
}

unsigned int Renderer::CreateShader(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    return program;
}

float Renderer::SimpleNoise(float x, float y) {
    int xi = (int)x & 255;
    int yi = (int)y & 255;
    float xf = x - (int)x;
    float yf = y - (int)y;

    // Simple hash function
    int a = (xi + yi * 57) * 131;
    int b = ((xi + 1) + yi * 57) * 131;
    int c = (xi + (yi + 1) * 57) * 131;
    int d = ((xi + 1) + (yi + 1) * 57) * 131;

    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);

    float n1 = (float)(a & 255) / 255.0f;
    float n2 = (float)(b & 255) / 255.0f;
    float n3 = (float)(c & 255) / 255.0f;
    float n4 = (float)(d & 255) / 255.0f;

    float i1 = n1 * (1.0f - u) + n2 * u;
    float i2 = n3 * (1.0f - u) + n4 * u;

    return i1 * (1.0f - v) + i2 * v;
}

void Renderer::CreateGrid() {
    // Create a large noise-displaced grid (terrain) centered at origin on XZ plane
    const int vertsPerSide = 256;  // 256x256 grid
    const float cellSize = 0.2f;   // 256 * 0.2 = ~51.2 units per side
    const float half = (vertsPerSide - 1) * cellSize * 0.5f;

    // Each vertex: position (3) + color (3)
    const int vertexStride = 6;
    const int vertexCount = vertsPerSide * vertsPerSide;
    const int quadCount = (vertsPerSide - 1) * (vertsPerSide - 1);
    const int indexCount = quadCount * 6;

    float* vertices = new float[vertexCount * vertexStride];
    unsigned int* indices = new unsigned int[indexCount];

    // Generate vertices with noise height on Y and color by height
    int v = 0;
    for (int z = 0; z < vertsPerSide; ++z) {
        for (int x = 0; x < vertsPerSide; ++x) {
            float worldX = -half + x * cellSize;
            float worldZ = -half + z * cellSize;
            float n = 0.0f;
            // FBM-like layered noise for nicer terrain
            float fx = x * 0.08f;
            float fz = z * 0.08f;
            float amp = 1.0f;
            float freq = 1.0f;
            for (int o = 0; o < 4; ++o) {
                n += SimpleNoise(fx * freq, fz * freq) * amp;
                freq *= 2.0f;
                amp *= 0.5f;
            }
            n = (n / (1.0f + 0.5f + 0.25f + 0.125f));  // normalize approx 0..1
            float height = (n - 0.5f) * 4.0f;          // scale to -2..2

            // position
            vertices[v++] = worldX;  // x
            vertices[v++] = height;  // y
            vertices[v++] = worldZ;  // z
            // color based on height: low=blueish, mid=green, high=brownish
            float r = 0, g = 0, b = 0;
            if (height < -0.5f) {
                r = 0.1f;
                g = 0.2f;
                b = 0.6f;
            } else if (height < 0.3f) {
                r = 0.1f;
                g = 0.6f;
                b = 0.2f;
            } else {
                r = 0.5f;
                g = 0.35f;
                b = 0.2f;
            }
            vertices[v++] = r;
            vertices[v++] = g;
            vertices[v++] = b;
        }
    }

    // Generate indices
    int k = 0;
    for (int z = 0; z < vertsPerSide - 1; ++z) {
        for (int x = 0; x < vertsPerSide - 1; ++x) {
            unsigned int i0 = z * vertsPerSide + x;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + vertsPerSide;
            unsigned int i3 = i2 + 1;
            indices[k++] = i0;
            indices[k++] = i2;
            indices[k++] = i1;
            indices[k++] = i1;
            indices[k++] = i2;
            indices[k++] = i3;
        }
    }

    gridIndicesCount = indexCount;

    unsigned int VBO, EBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &VBO);
    terrainVBO = VBO;
    glGenBuffers(1, &EBO);
    terrainEBO = EBO;

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(
        GL_ARRAY_BUFFER, vertexCount * vertexStride * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, vertexStride * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    delete[] vertices;
    delete[] indices;
}

void Renderer::CreateCube() {
    float vertices[] = {-0.1f, -0.1f, -0.1f, 0.0f, 0.0f,  0.0f,  0.1f, -0.1f, -0.1f, 0.0f,
                        0.0f,  0.0f,  0.1f,  0.1f, -0.1f, 0.0f,  0.0f, 0.0f,  -0.1f, 0.1f,
                        -0.1f, 0.0f,  0.0f,  0.0f, -0.1f, -0.1f, 0.1f, 0.0f,  0.0f,  0.0f,
                        0.1f,  -0.1f, 0.1f,  0.0f, 0.0f,  0.0f,  0.1f, 0.1f,  0.1f,  0.0f,
                        0.0f,  0.0f,  -0.1f, 0.1f, 0.1f,  0.0f,  0.0f, 0.0f};

    unsigned int indices[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 1, 5, 5, 4, 0,
                              2, 3, 7, 7, 6, 2, 0, 3, 7, 7, 4, 0, 1, 2, 6, 6, 5, 1};

    unsigned int VBO, EBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::CreateCrosshair() {
    float vertices[] = {-0.01f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f,   0.01f, 0.0f,
                        0.0f,   1.0f, 1.0f, 1.0f,  0.0f, -0.01f, 0.0f,  1.0f,
                        1.0f,   1.0f, 0.0f, 0.01f, 0.0f, 1.0f,   1.0f,  1.0f};

    unsigned int VBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Create tracer line with dynamic buffer
    glGenVertexArrays(1, &tracerVAO);
    glGenBuffers(1, &tracerVBO);

    glBindVertexArray(tracerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::UpdateTracerNDC(float x1, float y1, float x2, float y2) {
    // 2D overlay line in NDC with white color
    float vertices[] = {x1, y1, 0.0f, 1.0f, 1.0f, 1.0f, x2, y2, 0.0f, 1.0f, 1.0f, 1.0f};
    glBindBuffer(GL_ARRAY_BUFFER, tracerVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}

void Renderer::Render(const Camera& camera, Color& color) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Create view matrix
    float cosYaw = cos(camera.yaw), sinYaw = sin(camera.yaw);
    float cosPitch = cos(camera.pitch), sinPitch = sin(camera.pitch);

    float viewMatrix[16] = {
        cosYaw,
        sinPitch * sinYaw,
        -cosPitch * sinYaw,
        0.0f,
        0.0f,
        cosPitch,
        sinPitch,
        0.0f,
        sinYaw,
        -sinPitch * cosYaw,
        cosPitch * cosYaw,
        0.0f,
        -camera.x * cosYaw - camera.z * sinYaw,
        -camera.x * sinPitch * sinYaw - camera.y * cosPitch + camera.z * sinPitch * cosYaw,
        camera.x * cosPitch * sinYaw - camera.y * sinPitch - camera.z * cosPitch * cosYaw,
        1.0f};

    float projMatrix[16] = {1.5f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            2.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            -1.002f,
                            -1.0f,
                            0.0f,
                            0.0f,
                            -0.2002f,
                            0.0f};

    glUseProgram(shaderProgram);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);

    // Draw terrain grid
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(gridVAO);
    glDrawElements(GL_TRIANGLES, gridIndicesCount, GL_UNSIGNED_INT, 0);

    // Draw cube
    glUniform3f(colorLoc, color.r, color.g, color.b);
    glBindVertexArray(cubeVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Compute cube center in world space (cube at origin)
    float cubeWorld[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    // Multiply by view then projection to get clip space
    auto mul4x4 = [](const float m[16], const float v[4], float out[4]) {
        out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
        out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
        out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
        out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
    };
    float clip1[4];
    float clip2[4];
    mul4x4(viewMatrix, cubeWorld, clip1);
    mul4x4(projMatrix, clip1, clip2);
    float ndcCubeX = clip2[0] / clip2[3];
    float ndcCubeY = clip2[1] / clip2[3];

    // Get cursor position in client area to NDC using actual GL viewport
    GLint viewport[4];
    glGetIntegerv(0x0BA2 /* GL_VIEWPORT */, viewport);
    int vpX = viewport[0];
    int vpY = viewport[1];
    int vpW = viewport[2];
    int vpH = viewport[3];
    POINT p;
    GetCursorPos(&p);
    HWND win = WindowFromDC(hdc);
    if (!win)
        return;
    ScreenToClient(win, &p);
    // Client Y origin is top-left; convert to bottom-left for GL mapping
    RECT rc;
    GetClientRect(win, &rc);
    int pGLY = (rc.bottom - p.y) - vpY;  // relative to GL bottom-left
    int pGLX = p.x - vpX;
    if (pGLX < 0) {
        pGLX = 0;
    }
    if (pGLX > vpW) {
        pGLX = vpW;
    }
    if (pGLY < 0) {
        pGLY = 0;
    }
    if (pGLY > vpH) {
        pGLY = vpH;
    }
    float ndcCursorX = (pGLX / (float)vpW) * 2.0f - 1.0f;
    float ndcCursorY = (pGLY / (float)vpH) * 2.0f - 1.0f;

    // Update and draw tracer as overlay in screen space
    UpdateTracerNDC(ndcCursorX, ndcCursorY, ndcCubeX, ndcCubeY);
    float identityMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, identityMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, identityMatrix);
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(tracerVAO);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw crosshair (screen space)
    // identity matrices already set
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(crosshairVAO);
    glDrawArrays(GL_LINES, 0, 4);
    // Reset OpenGL state
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

#ifdef USE_IMGUI
    // Setup OpenGL state for ImGui
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    RenderImGui(camera, color);

    // Restore OpenGL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
#endif
}

void Renderer::Cleanup() {
#ifdef USE_IMGUI
    ShutdownImGui();
#endif
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(GetActiveWindow(), hdc);
}

void Renderer::Present() {
    SwapBuffers(hdc);
}

#ifdef USE_IMGUI
void Renderer::InitImGui(HWND hwnd) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    // Pick a safe GLSL version string based on current GL version to avoid invisible UI due to
    // shader compile failure
    const char* glsl_version = "#version 130";  // default safe choice for GL 3.0+
    const char* gl_ver = (const char*)glGetString(GL_VERSION);
    if (gl_ver) {
        int major = 0, minor = 0;
        // Parse strings like "4.6.0 ..." or "3.3.0 ..." or "2.1.0 ..."
        if (sscanf(gl_ver, "%d.%d", &major, &minor) == 2) {
            if (major < 3) {
                glsl_version = "#version 120";  // GL 2.1
            } else if (major == 3 && minor == 0) {
                glsl_version = "#version 130";  // GL 3.0
            } else if (major == 3 && minor == 1) {
                glsl_version = "#version 140";  // GL 3.1
            } else if (major == 3 && minor == 2) {
                glsl_version = "#version 150";  // GL 3.2
            } else if (major >= 3) {
                glsl_version =
                    "#version 150";  // good for 3.2+; backend does not require core profile
            }
        }
    }
    LOG_INFO("ImGui GL3 init with GLSL: %s", glsl_version);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Renderer::RenderImGui(const Camera& camera, Color& color) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Controls window
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");
    ImGui::Text("Camera: (%.2f, %.2f, %.2f)", camera.x, camera.y, camera.z);
    ImGui::ColorEdit3("Cube Color", &color.r);
    ImGui::Text("Press ESC to toggle mouse capture.");
    ImGui::End();

    // Log window
    static bool show_logs = true;
    if (show_logs) {
        ImGui::SetNextWindowPos(ImVec2(10, 170), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Logs", &show_logs);

        static bool auto_scroll = true;
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            // Clear is handled by getting fresh logs
        }

        ImGui::Separator();
        ImGui::BeginChild(
            "LogScrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        auto logs = logging::GetRecentLogs(200);
        for (const auto& log : logs) {
            ImGui::TextUnformatted(log.c_str());
        }

        if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
#endif