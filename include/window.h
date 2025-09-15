#pragma once
#include <windows.h>

class Window {
  public:
    bool Create(HINSTANCE hInstance, int nCmdShow);
    void Run();
    HWND GetHandle() const {
        return hwnd;
    }

  private:
    HWND hwnd;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};