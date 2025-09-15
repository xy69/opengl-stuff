#include "window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>

#include "logger.h"
#include "renderer.h"

#ifdef USE_IMGUI
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#endif

static Renderer renderer;
static Camera camera = {0.0f, 0.5f, -2.0f, 0.0f, 0.0f, 0.02f};
static Color currentColor = {1.0f, 0.5f, 0.0f};

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Update the GL viewport to match the framebuffer size to avoid distortion
    glViewport(0, 0, width, height);
}

bool Window::Create(int width, int height, const char* title) {
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW");
        return false;
    }

#if __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        LOG_ERROR("Failed to initialize GLEW");
        return false;
    }

    glfwSwapInterval(1);  // vsync

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetWindowUserPointer(window, this);

    LOG_INFO("Initializing renderer");
    renderer.Initialize(window);

#ifdef USE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
#endif

    // Start with mouse captured for FPS-style look; user can toggle with ESC
    ToggleMouseCapture(true);

    return true;
}

void Window::ToggleMouseCapture(bool capture) {
    mouseCaptured = capture;
    if (mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Window::Run() {
    LOG_INFO("Entering main loop");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Keyboard movement
        float cosYaw = cosf(camera.yaw), sinYaw = sinf(camera.yaw);
        float cosPitch = cosf(camera.pitch);
        float frontX = cosYaw * cosPitch;
        float frontZ = sinYaw * cosPitch;
        float rightX = -sinYaw;
        float rightZ = cosYaw;

        auto key = [&](int k) { return glfwGetKey(window, k) == GLFW_PRESS; };
        if (key(GLFW_KEY_W)) {
            camera.x -= rightX * camera.speed;
            camera.z -= rightZ * camera.speed;
        }
        if (key(GLFW_KEY_S)) {
            camera.x += rightX * camera.speed;
            camera.z += rightZ * camera.speed;
        }
        if (key(GLFW_KEY_A)) {
            camera.x -= frontX * camera.speed;
            camera.z -= frontZ * camera.speed;
        }
        if (key(GLFW_KEY_D)) {
            camera.x += frontX * camera.speed;
            camera.z += frontZ * camera.speed;
        }
        if (key(GLFW_KEY_SPACE))
            camera.y += camera.speed;
        if (key(GLFW_KEY_LEFT_SHIFT))
            camera.y -= camera.speed;

        static bool escDown = false;
        if (key(GLFW_KEY_ESCAPE)) {
            if (!escDown) {
                escDown = true;
                ToggleMouseCapture(!mouseCaptured);
            }
        } else {
            escDown = false;
        }

        // Mouse look when captured
        if (mouseCaptured) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            float deltaX = float(x - lastMouseX) * 0.002f;
            float deltaY = float(y - lastMouseY) * 0.002f;
            camera.yaw += deltaX;
            camera.pitch += deltaY;
            if (camera.pitch > 1.5f)
                camera.pitch = 1.5f;
            if (camera.pitch < -1.5f)
                camera.pitch = -1.5f;
            lastMouseX = x;
            lastMouseY = y;
        }

#ifdef USE_IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_FirstUseEver);
        ImGui::Begin("Controls");
        ImGui::Text("Camera: (%.2f, %.2f, %.2f)", camera.x, camera.y, camera.z);
        ImGui::ColorEdit3("Cube Color", &currentColor.r);
        ImGui::Text("Press ESC to toggle mouse capture.");
        ImGui::End();

        static bool show_logs = true;
        if (show_logs) {
            ImGui::SetNextWindowPos(ImVec2(10, 170), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
            ImGui::Begin("Logs", &show_logs);

            static bool auto_scroll = true;
            ImGui::Checkbox("Auto-scroll", &auto_scroll);
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
#endif

        renderer.Render(camera, currentColor);

#ifdef USE_IMGUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

        glfwSwapBuffers(window);
    }
    LOG_INFO("Exiting main loop");
    renderer.Cleanup();
#ifdef USE_IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif
    glfwDestroyWindow(window);
    glfwTerminate();
}