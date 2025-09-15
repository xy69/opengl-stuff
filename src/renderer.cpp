#include "renderer.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>

#include "logger.h"

#ifdef USE_IMGUI
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#endif
bool Renderer::Initialize(GLFWwindow* win) {
    srand((unsigned int)time(nullptr));
    window = win;

    const char* gl_version = (const char*)glGetString(GL_VERSION);
    const char* gl_renderer = (const char*)glGetString(GL_RENDERER);
    const char* gl_vendor = (const char*)glGetString(GL_VENDOR);
    LOG_INFO("OpenGL Version: %s", gl_version ? gl_version : "<null>");
    LOG_INFO("OpenGL Renderer: %s", gl_renderer ? gl_renderer : "<null>");
    LOG_INFO("OpenGL Vendor: %s", gl_vendor ? gl_vendor : "<null>");

    shaderProgram = CreateShaderFromFiles("terrain.vert", "terrain.frag");
    CreateGrid();
    CreateCube();
    CreateCrosshair();

    glEnable(GL_DEPTH_TEST);

    // VSync handled via glfwSwapInterval in window

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

std::string Renderer::ReadTextFile(const char* path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
        return {};
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

unsigned int Renderer::CreateShaderFromFiles(const char* vertexPath, const char* fragmentPath) {
#ifdef SHADER_DIR
    std::string vpath = std::string(SHADER_DIR) + "/" + vertexPath;
    std::string fpath = std::string(SHADER_DIR) + "/" + fragmentPath;
#else
    std::string vpath = vertexPath;
    std::string fpath = fragmentPath;
#endif
    std::string vsrc = ReadTextFile(vpath.c_str());
    std::string fsrc = ReadTextFile(fpath.c_str());
    if (vsrc.empty() || fsrc.empty()) {
        LOG_ERROR("Failed to load shaders: %s, %s", vpath.c_str(), fpath.c_str());
        // Fallback minimal shaders
        const char* vs =
            "#version 330 core\nlayout(location=0) in vec3 aPos; layout(location=1) in vec3 "
            "aColor; uniform mat4 uView; uniform mat4 uProjection; uniform vec3 uColor; out vec3 "
            "vertexColor; void main(){ gl_Position=uProjection*uView*vec4(aPos,1.0); "
            "vertexColor=aColor*uColor; }";
        const char* fs =
            "#version 330 core\nin vec3 vertexColor; out vec4 FragColor; void main(){ "
            "FragColor=vec4(vertexColor,1.0); }";
        return CreateShader(vs, fs);
    }
    return CreateShader(vsrc.c_str(), fsrc.c_str());
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
    // Initialize an empty VBO; we'll fill it each frame to maintain fixed pixel size
    float vertices[24] = {0};
    unsigned int VBO;
    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &VBO);
    crosshairVBO = VBO;

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

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

    // Build perspective projection based on current viewport aspect ratio
    GLint viewportForProj[4];
    glGetIntegerv(GL_VIEWPORT, viewportForProj);
    int projW = viewportForProj[2];
    int projH = viewportForProj[3];
    float aspect = (projH != 0) ? (float)projW / (float)projH : 1.0f;
    float fovY = 60.0f * (3.1415926535f / 180.0f);
    float f = 1.0f / tanf(fovY * 0.5f);
    float zNear = 0.1f;
    float zFar = 100.0f;
    float A = (zFar + zNear) / (zNear - zFar);
    float B = (2.0f * zFar * zNear) / (zNear - zFar);
    float projMatrix[16] = {f / aspect,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            A,
                            -1.0f,
                            0.0f,
                            0.0f,
                            B,
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

    // Get cursor position in client area to NDC using actual GL viewport (GLFW)
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int vpX = viewport[0];
    int vpY = viewport[1];
    int vpW = viewport[2];
    int vpH = viewport[3];
    float ndcCursorX = 0.0f, ndcCursorY = 0.0f;
    int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
    if (cursorMode != GLFW_CURSOR_DISABLED) {
        // Cursor visible: map to NDC with HiDPI scaling
        double cx = 0.0, cy = 0.0;
        glfwGetCursorPos(window, &cx, &cy);
        int winW = 0, winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        double scaleX = winW > 0 ? (double)fbW / (double)winW : 1.0;
        double scaleY = winH > 0 ? (double)fbH / (double)winH : 1.0;
        double px = cx * scaleX;
        double py = cy * scaleY;
        // Convert to GL bottom-left origin and account for viewport offset
        double pGLX = px - vpX;
        double pGLY = (fbH - py) - vpY;
        if (pGLX < 0)
            pGLX = 0;
        if (pGLX > vpW)
            pGLX = vpW;
        if (pGLY < 0)
            pGLY = 0;
        if (pGLY > vpH)
            pGLY = vpH;
        ndcCursorX = (float)(pGLX / (double)vpW * 2.0 - 1.0);
        ndcCursorY = (float)(pGLY / (double)vpH * 2.0 - 1.0);
    } else {
        // Cursor captured: tracer should originate from crosshair at the screen center
        ndcCursorX = 0.0f;
        ndcCursorY = 0.0f;
    }

    // Update and draw tracer as overlay in screen space (disable depth test so it draws on top)
    UpdateTracerNDC(ndcCursorX, ndcCursorY, ndcCubeX, ndcCubeY);
    float identityMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, identityMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, identityMatrix);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled)
        glDisable(GL_DEPTH_TEST);
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glBindVertexArray(tracerVAO);
    glDrawArrays(GL_LINES, 0, 2);

    // Draw crosshair (screen space) with fixed pixel size regardless of aspect
    // Compute NDC size based on current viewport dimensions
    GLint vp2[4];
    glGetIntegerv(GL_VIEWPORT, vp2);
    int vpW2 = vp2[2];
    int vpH2 = vp2[3];
    // Desired crosshair half-length in pixels
    const float halfLenPx = 8.0f;
    float dx = (vpW2 > 0) ? (halfLenPx / (float)vpW2) * 2.0f : 0.02f;
    float dy = (vpH2 > 0) ? (halfLenPx / (float)vpH2) * 2.0f : 0.02f;
    // Two lines centered at origin
    float ch[24] = {
        -dx,  0.0f, 0.0f, 1, 1, 1, dx,   0.0f, 0.0f, 1, 1, 1,
        0.0f, -dy,  0.0f, 1, 1, 1, 0.0f, dy,   0.0f, 1, 1, 1,
    };
    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ch), ch);
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, 0, 4);
    // Reset OpenGL state
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);

    // ImGui UI pass (if enabled) handled from main loop or here optionally
}

void Renderer::Cleanup() {
    // Nothing to do here for GLFW-managed context
}
