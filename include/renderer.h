#pragma once

// GLEW provides OpenGL function declarations
#include <GL/glew.h>

#include <string>

struct GLFWwindow;

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
    bool Initialize(GLFWwindow* window);
    void Render(const Camera& camera, Color& color);
    void Cleanup();

  private:
    GLFWwindow* window = nullptr;
    unsigned int shaderProgram;
    unsigned int gridVAO, cubeVAO, crosshairVAO, crosshairVBO, tracerVAO, tracerVBO;
    int gridIndicesCount;

    // Terrain buffers
    unsigned int terrainVBO = 0, terrainEBO = 0;

    unsigned int CreateShader(const char* vertexSource, const char* fragmentSource);
    unsigned int CreateShaderFromFiles(const char* vertexPath, const char* fragmentPath);
    static std::string ReadTextFile(const char* path);
    void CreateGrid();
    void CreateCube();
    void CreateCrosshair();
    // Update tracer line in screen space (NDC). Endpoints are in range [-1,1].
    void UpdateTracerNDC(float x1, float y1, float x2, float y2);
    float SimpleNoise(float x, float y);
};