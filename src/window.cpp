#include "window.h"

#include <GL/gl.h>
#include <shellapi.h>
#include <windows.h>

#include <cmath>
#include <cstdio>

#include "logger.h"
#include "renderer.h"

#ifdef USE_IMGUI
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
#endif

static Renderer renderer;
static Camera camera = {0.0f, 0.5f, -2.0f, 0.0f, 0.0f, 0.02f};
static Color currentColor = {1.0f, 0.5f, 0.0f};
static bool mouseCaptured = false;  // Start with mouse free for ImGui
static int lastMouseX = 400, lastMouseY = 300;

// Legacy button IDs removed; ImGui is used instead when enabled

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#ifdef USE_IMGUI
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;
#endif

    switch (uMsg) {
        case WM_DESTROY:
            LOG_INFO("WM_DESTROY received, quitting");
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND:
            return 0;

        case WM_LBUTTONDOWN:
#ifdef USE_IMGUI
            // Don't capture mouse if ImGui wants it
            if (ImGui::GetIO().WantCaptureMouse) {
                return 0;
            }
#endif
            if (!mouseCaptured) {
                mouseCaptured = true;
                LOG_INFO("Mouse captured");
                ShowCursor(FALSE);
                RECT rect;
                GetClientRect(hwnd, &rect);
                ClientToScreen(hwnd, (POINT*)&rect.left);
                ClientToScreen(hwnd, (POINT*)&rect.right);
                ClipCursor(&rect);
                SetCapture(hwnd);
            }
            return 0;

        case WM_MOUSEMOVE:
            if (mouseCaptured) {
                int mouseX = LOWORD(lParam);
                int mouseY = HIWORD(lParam);

                float deltaX = (mouseX - lastMouseX) * 0.002f;
                float deltaY = (mouseY - lastMouseY) * 0.002f;

                camera.yaw += deltaX;
                camera.pitch += deltaY;

                if (camera.pitch > 1.5f)
                    camera.pitch = 1.5f;
                if (camera.pitch < -1.5f)
                    camera.pitch = -1.5f;

                // Recenter to the client area center
                RECT rect;
                GetClientRect(hwnd, &rect);
                lastMouseX = rect.right / 2;
                lastMouseY = rect.bottom / 2;
                POINT center = {lastMouseX, lastMouseY};
                ClientToScreen(hwnd, &center);
                SetCursorPos(center.x, center.y);
            }
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                LOG_INFO("WM_SIZE viewport %dx%d", rect.right, rect.bottom);
                glViewport(0, 0, rect.right, rect.bottom);
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Window::Create(HINSTANCE hInstance, int nCmdShow) {
#ifdef USE_IMGUI
    LOG_INFO("[window.cpp] USE_IMGUI is defined");
#else
    LOG_WARN("[window.cpp] USE_IMGUI is NOT defined");
#endif
    const char* className = "OpenGLWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    hwnd = CreateWindowEx(0,
                          className,
                          "OpenGL Terrain",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          800,
                          600,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    if (!hwnd) {
        LOG_ERROR("CreateWindowEx failed");
        return false;
    }

    ShowWindow(hwnd, nCmdShow);

    LOG_INFO("Initializing renderer");
    renderer.Initialize(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);
    glViewport(0, 0, rect.right, rect.bottom);

    return true;
}

void Window::Run() {
    LOG_INFO("Entering main loop");
    MSG msg = {};
    bool running = true;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                LOG_INFO("WM_QUIT received");
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Handle movement
        float cosYaw = cos(camera.yaw), sinYaw = sin(camera.yaw);
        float cosPitch = cos(camera.pitch);

        float frontX = cosYaw * cosPitch;
        float frontZ = sinYaw * cosPitch;
        float rightX = -sinYaw;
        float rightZ = cosYaw;

        if (GetAsyncKeyState('W') & 0x8000) {
            camera.x -= rightX * camera.speed;
            camera.z -= rightZ * camera.speed;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            camera.x += rightX * camera.speed;
            camera.z += rightZ * camera.speed;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            camera.x -= frontX * camera.speed;
            camera.z -= frontZ * camera.speed;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            camera.x += frontX * camera.speed;
            camera.z += frontZ * camera.speed;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
            camera.y += camera.speed;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            camera.y -= camera.speed;

        // Handle escape to toggle mouse capture for ImGui
        static bool escapePressed = false;
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            if (!escapePressed) {
                escapePressed = true;
                if (mouseCaptured) {
                    mouseCaptured = false;
                    LOG_INFO("Mouse released for ImGui");
                    ShowCursor(TRUE);
                    ClipCursor(NULL);
                    ReleaseCapture();
                } else {
                    mouseCaptured = true;
                    LOG_INFO("Mouse recaptured");
                    ShowCursor(FALSE);
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    ClientToScreen(hwnd, (POINT*)&rect.left);
                    ClientToScreen(hwnd, (POINT*)&rect.right);
                    ClipCursor(&rect);
                    SetCapture(hwnd);
                }
            }
        } else {
            escapePressed = false;
        }

        renderer.Render(camera, currentColor);
        renderer.Present();
    }
    LOG_INFO("Exiting main loop");
    renderer.Cleanup();
}