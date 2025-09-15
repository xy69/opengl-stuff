#pragma once

struct GLFWwindow;  // forward declaration

class Window {
  public:
    // Create a window and OpenGL context
    bool Create(int width = 800, int height = 600, const char* title = "OpenGL Terrain");
    // Main loop
    void Run();
    // Access to the native GLFW window pointer
    GLFWwindow* GetHandle() const {
        return window;
    }

  private:
    GLFWwindow* window = nullptr;
    bool mouseCaptured = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    void ToggleMouseCapture(bool capture);
};