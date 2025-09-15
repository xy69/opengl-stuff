#include "logger.h"
#include "window.h"

int main() {
    logging::Initialize();
    LOG_INFO("Application start");
#ifdef USE_IMGUI
    LOG_INFO("[main.cpp] USE_IMGUI is defined");
#else
    LOG_WARN("[main.cpp] USE_IMGUI is NOT defined");
#endif

    Window window;
    if (!window.Create()) {
        LOG_ERROR("Window creation failed");
        return -1;
    }

    window.Run();
    LOG_INFO("Application exit");
    logging::Shutdown();
    return 0;
}