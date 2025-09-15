#pragma once
#include <GL/gl.h>
#include <windows.h>

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#endif

struct Camera {
    float x, y, z;
    float pitch, yaw;
    float speed;
};

struct Color {
    float r, g, b;
};

class Renderer {
  public:
    bool Initialize(HWND hwnd);
    void Render(const Camera& camera, Color& color);
    void Present();
    void Cleanup();

  private:
    HDC hdc;
    HGLRC hglrc;
    unsigned int shaderProgram;
    unsigned int gridVAO, cubeVAO, crosshairVAO, tracerVAO, tracerVBO;
    int gridIndicesCount;

    // Terrain buffers
    unsigned int terrainVBO = 0, terrainEBO = 0;

    void LoadOpenGLFunctions();
    unsigned int CreateShader(const char* vertexSource, const char* fragmentSource);
    void CreateGrid();
    void CreateCube();
    void CreateCrosshair();
    // Update tracer line in screen space (NDC). Endpoints are in range [-1,1].
    void UpdateTracerNDC(float x1, float y1, float x2, float y2);
    float SimpleNoise(float x, float y);

#ifdef USE_IMGUI
    void InitImGui(HWND hwnd);
    void RenderImGui(const Camera& camera, Color& color);
    void ShutdownImGui();
#endif
};