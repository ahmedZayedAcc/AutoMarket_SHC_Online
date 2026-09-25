#pragma once

#include <windows.h>

namespace Menu
{
    void Init(HWND hwnd);
    void Uninit();

    void SetContext(HDC hdc);

    void SetSurfaceSize(
        int width,
        int height
    );

    void Draw();

    bool IsVisible();

    LRESULT ImplWin32_WndProcHandler(
        WNDPROC oWndProc,
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
    );
}
