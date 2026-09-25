extern "C" int __mingw_SEH_error_handler(
    void*,
    void*,
    void*,
    void*
)
{
    return 1;
}

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <cwchar>

#include <Windows.h>
#include <ddraw.h>

#include "MinHook.h"
#include "log.hpp"
#include "internal.hpp"
#include "menu.hpp"
#include "config.hpp"


// ============================================================
// Real DirectDraw
// ============================================================

static HMODULE GetRealDDraw()
{
    static HMODULE hReal =
    []() -> HMODULE
    {
        wchar_t path[MAX_PATH] = {};

        GetSystemDirectoryW(
            path,
            MAX_PATH
        );

        std::wstring ddrawPath =
            std::wstring(path)
            +
            L"\\ddraw.dll";

        return LoadLibraryW(
            ddrawPath.c_str()
        );
    }();

    return hReal;
}


// ============================================================
// Exported DirectDraw functions
// ============================================================

extern "C"
__declspec(dllexport)
HRESULT WINAPI DirectDrawCreate(
    GUID* lpGUID,
    LPDIRECTDRAW* lplpDD,
    IUnknown* pUnkOuter
)
{
    typedef HRESULT(WINAPI* Fn)(
        GUID*,
        LPDIRECTDRAW*,
        IUnknown*
    );

    static Fn realFn =
        (Fn)GetProcAddress(
            GetRealDDraw(),
            "DirectDrawCreate"
        );

    if (!realFn)
        return E_FAIL;

    return realFn(
        lpGUID,
        lplpDD,
        pUnkOuter
    );
}


extern "C"
__declspec(dllexport)
HRESULT WINAPI DirectDrawCreateEx(
    GUID* lpGUID,
    LPVOID* lplpDD,
    REFIID iid,
    IUnknown* pUnkOuter
)
{
    typedef HRESULT(WINAPI* Fn)(
        GUID*,
        LPVOID*,
        REFIID,
        IUnknown*
    );

    static Fn realFn =
        (Fn)GetProcAddress(
            GetRealDDraw(),
            "DirectDrawCreateEx"
        );

    if (!realFn)
        return E_FAIL;

    return realFn(
        lpGUID,
        lplpDD,
        iid,
        pUnkOuter
    );
}


extern "C"
__declspec(dllexport)
HRESULT WINAPI DirectDrawEnumerateA(
    LPDDENUMCALLBACKA lpCallback,
    LPVOID lpContext
)
{
    typedef HRESULT(WINAPI* Fn)(
        LPDDENUMCALLBACKA,
        LPVOID
    );

    static Fn realFn =
        (Fn)GetProcAddress(
            GetRealDDraw(),
            "DirectDrawEnumerateA"
        );

    if (!realFn)
        return E_FAIL;

    return realFn(
        lpCallback,
        lpContext
    );
}


extern "C"
__declspec(dllexport)
HRESULT WINAPI DirectDrawEnumerateW(
    LPDDENUMCALLBACKW lpCallback,
    LPVOID lpContext
)
{
    typedef HRESULT(WINAPI* Fn)(
        LPDDENUMCALLBACKW,
        LPVOID
    );

    static Fn realFn =
        (Fn)GetProcAddress(
            GetRealDDraw(),
            "DirectDrawEnumerateW"
        );

    if (!realFn)
        return E_FAIL;

    return realFn(
        lpCallback,
        lpContext
    );
}


// ============================================================
// Function typedefs
// ============================================================

typedef HWND(WINAPI* CreateWindowExAFn)(
    DWORD,
    LPCSTR,
    LPCSTR,
    DWORD,
    int,
    int,
    int,
    int,
    HWND,
    HMENU,
    HINSTANCE,
    LPVOID
);


typedef HWND(WINAPI* CreateWindowExWFn)(
    DWORD,
    LPCWSTR,
    LPCWSTR,
    DWORD,
    int,
    int,
    int,
    int,
    HWND,
    HMENU,
    HINSTANCE,
    LPVOID
);


typedef HRESULT(WINAPI* DirectDrawCreateFn)(
    GUID*,
    IDirectDraw**,
    IUnknown*
);


typedef HRESULT(STDMETHODCALLTYPE* CreateSurfaceFn)(
    IDirectDraw*,
    LPDDSURFACEDESC,
    LPDIRECTDRAWSURFACE*,
    IUnknown*
);


typedef HRESULT(STDMETHODCALLTYPE* BltFn)(
    IDirectDrawSurface*,
    LPCRECT,
    IDirectDrawSurface*,
    LPCRECT,
    DWORD,
    LPDDBLTFX
);


// ============================================================
// Hook pointers
// ============================================================

CreateWindowExAFn
pCreateWindowExA = nullptr;

CreateWindowExAFn
oCreateWindowExA = nullptr;


CreateWindowExWFn
pCreateWindowExW = nullptr;

CreateWindowExWFn
oCreateWindowExW = nullptr;


DirectDrawCreateFn
pDirectDrawCreate = nullptr;

DirectDrawCreateFn
oDirectDrawCreate = nullptr;


CreateSurfaceFn
pCreateSurface = nullptr;

CreateSurfaceFn
oCreateSurface = nullptr;


BltFn
pBlt = nullptr;

BltFn
oBlt = nullptr;


// ============================================================
// Global state
// ============================================================

WNDPROC oWndProc = nullptr;

GameInterface driver;

HWND g_hwnd = nullptr;


std::chrono::time_point<
    std::chrono::high_resolution_clock
>
lastTradeTime;


bool wasInGame = false;


// ============================================================
// Forward declarations
// ============================================================

LRESULT __stdcall MyWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
);


HWND WINAPI MyCreateWindowExA(
    DWORD,
    LPCSTR,
    LPCSTR,
    DWORD,
    int,
    int,
    int,
    int,
    HWND,
    HMENU,
    HINSTANCE,
    LPVOID
);


HWND WINAPI MyCreateWindowExW(
    DWORD,
    LPCWSTR,
    LPCWSTR,
    DWORD,
    int,
    int,
    int,
    int,
    HWND,
    HMENU,
    HINSTANCE,
    LPVOID
);


HRESULT WINAPI MyDirectDrawCreate(
    GUID*,
    IDirectDraw**,
    IUnknown*
);


HRESULT STDMETHODCALLTYPE MyCreateSurface(
    IDirectDraw*,
    LPDDSURFACEDESC,
    LPDIRECTDRAWSURFACE*,
    IUnknown*
);


HRESULT STDMETHODCALLTYPE MyBlt(
    IDirectDrawSurface*,
    LPCRECT,
    IDirectDrawSurface*,
    LPCRECT,
    DWORD,
    LPDDBLTFX
);


// ============================================================
// Utility
// ============================================================

bool ContainsWordIgnoreCase(
    const std::string& str,
    const std::string& word
)
{
    std::string ls =
        str;

    std::string lw =
        word;


    std::transform(
        ls.begin(),
        ls.end(),
        ls.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c)
            );
        }
    );


    std::transform(
        lw.begin(),
        lw.end(),
        lw.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c)
            );
        }
    );


    return
        ls.find(lw)
        !=
        std::string::npos;
}


// ============================================================
// Execute Trade
// ============================================================

void ExecuteTrade()
{
    bool currentlyInGame =
        driver.isInGame();


    if (
        wasInGame
        &&
        !currentlyInGame
    )
    {
        ConfigManager::Instance()
            .ResetAll();


        wasInGame =
            false;


        return;
    }


    if (
        !currentlyInGame
    )
    {
        wasInGame =
            false;


        return;
    }


    wasInGame =
        true;


    auto now =
        std::chrono::high_resolution_clock::now();


    int freqMs =
        ConfigManager::Instance()
            .GetTradeFrequency();


    if (
        freqMs <= 0
    )
    {
        freqMs =
            20000;
    }


    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            now
            -
            lastTradeTime
        ).count();


    if (
        elapsed
        <
        freqMs
    )
    {
        return;
    }


    auto& cfg =
        ConfigManager::Instance();


    const auto& items =
        cfg.GetItems();


    for (
        const auto& item :
        items
    )
    {
        int count =
            driver.getNumberProducts(
                item.name
            );


        if (
            item.saleThreshold > 0
            &&
            count >
            item.saleThreshold
        )
        {
            driver.sellProduct(
                item.name
            );
        }


        if (
            item.buyThreshold > 0
            &&
            count <
            item.buyThreshold
        )
        {
            driver.buyProduct(
                item.name
            );
        }
    }


    lastTradeTime =
        std::chrono::high_resolution_clock::now();
}


// ============================================================
// DirectDrawCreate hook
// ============================================================

HRESULT WINAPI MyDirectDrawCreate(
    GUID* lpGUID,
    IDirectDraw** lplpDD,
    IUnknown* pUnkOuter
)
{
    HRESULT hr =
        oDirectDrawCreate(
            lpGUID,
            lplpDD,
            pUnkOuter
        );


    if (
        !SUCCEEDED(hr)
        ||
        !lplpDD
        ||
        !*lplpDD
    )
    {
        return hr;
    }


    void** vTable =
        *reinterpret_cast<void***>(
            *lplpDD
        );


    pCreateSurface =
        reinterpret_cast<CreateSurfaceFn>(
            vTable[6]
        );


    if (
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                pCreateSurface
            ),
            reinterpret_cast<LPVOID>(
                &MyCreateSurface
            ),
            reinterpret_cast<void**>(
                &oCreateSurface
            )
        )
        !=
        MH_OK
    )
    {
        return hr;
    }


    MH_EnableHook(
        reinterpret_cast<LPVOID>(
            pCreateSurface
        )
    );


    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            pDirectDrawCreate
        )
    );


    // FIX:
    // The reinterpret_cast and MH_RemoveHook
    // must each have their own closing parenthesis.

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            pDirectDrawCreate
        )
    );


    return hr;
}


// ============================================================
// CreateSurface hook
// ============================================================

HRESULT STDMETHODCALLTYPE MyCreateSurface(
    IDirectDraw* pDD,
    LPDDSURFACEDESC lpDesc,
    LPDIRECTDRAWSURFACE* lplpSurf,
    IUnknown* pUnk
)
{
    HRESULT hr =
        oCreateSurface(
            pDD,
            lpDesc,
            lplpSurf,
            pUnk
        );


    if (
        !SUCCEEDED(hr)
        ||
        !lplpSurf
        ||
        !*lplpSurf
    )
    {
        return hr;
    }


    void** vTable =
        *reinterpret_cast<void***>(
            *lplpSurf
        );


    pBlt =
        reinterpret_cast<BltFn>(
            vTable[5]
        );


    if (
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                pBlt
            ),
            reinterpret_cast<LPVOID>(
                &MyBlt
            ),
            reinterpret_cast<void**>(
                &oBlt
            )
        )
        !=
        MH_OK
    )
    {
        return hr;
    }


    if (
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                pBlt
            )
        )
        !=
        MH_OK
    )
    {
        MH_RemoveHook(
            reinterpret_cast<LPVOID>(
                pBlt
            )
        );


        return hr;
    }


    MH_DisableHook(
        reinterpret_cast<LPVOID>(
            pCreateSurface
        )
    );


    // FIX:
    // Missing ')' before ';' was causing the compiler error.

    MH_RemoveHook(
        reinterpret_cast<LPVOID>(
            pCreateSurface
        )
    );


    return hr;
}


// ============================================================
// Blt hook
// ============================================================

HRESULT STDMETHODCALLTYPE MyBlt(
    IDirectDrawSurface* pDest,
    LPCRECT lpDestRect,
    IDirectDrawSurface* pSrc,
    LPCRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFx
)
{
    // --------------------------------------------------------
    // AutoMarket
    // --------------------------------------------------------

    ExecuteTrade();


    // --------------------------------------------------------
    // Menu
    // --------------------------------------------------------

    if (
        Menu::IsVisible()
    )
    {
        IDirectDrawSurface*
            targetSurf =
                pDest;


        if (
            targetSurf
        )
        {
            DDSURFACEDESC desc =
                {};


            desc.dwSize =
                sizeof(desc);


            int surfaceW =
                0;

            int surfaceH =
                0;


            if (
                SUCCEEDED(
                    targetSurf->GetSurfaceDesc(
                        &desc
                    )
                )
            )
            {
                surfaceW =
                    static_cast<int>(
                        desc.dwWidth
                    );


                surfaceH =
                    static_cast<int>(
                        desc.dwHeight
                    );
            }


            HDC hDC =
                nullptr;


            if (
                SUCCEEDED(
                    targetSurf->GetDC(
                        &hDC
                    )
                )
                &&
                hDC
            )
            {
                SelectClipRgn(
                    hDC,
                    NULL
                );


                Menu::SetContext(
                    hDC
                );


                Menu::SetSurfaceSize(
                    surfaceW,
                    surfaceH
                );


                Menu::Draw();


                targetSurf->ReleaseDC(
                    hDC
                );
            }
        }
    }


    return oBlt(
        pDest,
        lpDestRect,
        pSrc,
        lpSrcRect,
        dwFlags,
        lpDDBltFx
    );
}


// ============================================================
// Main initialization
// ============================================================

bool MainInit(
    HWND hwnd
)
{
    g_hwnd =
        hwnd;


    wchar_t cwd[MAX_PATH] =
        {};


    GetCurrentDirectoryW(
        MAX_PATH,
        cwd
    );


    std::wstring iniPath =
        std::wstring(cwd)
        +
        L"\\automarket.ini";


    ConfigManager::Instance()
        .Load(
            iniPath
        );


    Menu::Init(
        g_hwnd
    );


    lastTradeTime =
        std::chrono::high_resolution_clock::now();


    wasInGame =
        false;


    oWndProc =
        reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(
                hwnd,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(
                    MyWndProc
                )
            )
        );


    return
        oWndProc
        !=
        nullptr;
}


// ============================================================
// CreateWindowExA
// ============================================================

HWND WINAPI MyCreateWindowExA(
    DWORD dwExStyle,
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam
)
{
    HWND hwnd =
        oCreateWindowExA(
            dwExStyle,
            lpClassName,
            lpWindowName,
            dwStyle,
            x,
            y,
            nWidth,
            nHeight,
            hWndParent,
            hMenu,
            hInstance,
            lpParam
        );


    if (
        hwnd
        &&
        !g_hwnd
    )
    {
        char title[256] =
            {};


        GetWindowTextA(
            hwnd,
            title,
            sizeof(title)
        );


        if (
            ContainsWordIgnoreCase(
                title,
                "Crusader"
            )
        )
        {
            MainInit(
                hwnd
            );


            if (
                pCreateWindowExA
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExA
                    )
                );


                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExA
                    )
                );
            }


            if (
                pCreateWindowExW
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExW
                    )
                );


                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExW
                    )
                );
            }
        }
    }


    return hwnd;
}


// ============================================================
// CreateWindowExW
// ============================================================

HWND WINAPI MyCreateWindowExW(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam
)
{
    HWND hwnd =
        oCreateWindowExW(
            dwExStyle,
            lpClassName,
            lpWindowName,
            dwStyle,
            x,
            y,
            nWidth,
            nHeight,
            hWndParent,
            hMenu,
            hInstance,
            lpParam
        );


    if (
        hwnd
        &&
        !g_hwnd
    )
    {
        wchar_t titleW[256] =
            {};


        GetWindowTextW(
            hwnd,
            titleW,
            256
        );


        char title[256] =
            {};


        WideCharToMultiByte(
            CP_ACP,
            0,
            titleW,
            -1,
            title,
            256,
            NULL,
            NULL
        );


        if (
            ContainsWordIgnoreCase(
                title,
                "Crusader"
            )
        )
        {
            MainInit(
                hwnd
            );


            if (
                pCreateWindowExW
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExW
                    )
                );


                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExW
                    )
                );
            }


            if (
                pCreateWindowExA
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExA
                    )
                );


                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        pCreateWindowExA
                    )
                );
            }
        }
    }


    return hwnd;
}


// ============================================================
// Setup hooks
// ============================================================

bool SetupHooks()
{
    if (
        MH_Initialize()
        !=
        MH_OK
    )
    {
        return false;
    }


    // ========================================================
    // Human Market trace
    // ========================================================

    driver.InstallHumanMarketTrace();


    // ========================================================
    // Real DirectDraw
    // ========================================================

    HMODULE hDdraw =
        GetRealDDraw();


    if (
        !hDdraw
    )
    {
        return false;
    }


    pDirectDrawCreate =
        reinterpret_cast<
            DirectDrawCreateFn
        >(
            GetProcAddress(
                hDdraw,
                "DirectDrawCreate"
            )
        );


    if (
        !pDirectDrawCreate
    )
    {
        return false;
    }


    if (
        MH_CreateHook(
            reinterpret_cast<LPVOID>(
                pDirectDrawCreate
            ),
            reinterpret_cast<LPVOID>(
                &MyDirectDrawCreate
            ),
            reinterpret_cast<void**>(
                &oDirectDrawCreate
            )
        )
        !=
        MH_OK
    )
    {
        return false;
    }


    if (
        MH_EnableHook(
            reinterpret_cast<LPVOID>(
                pDirectDrawCreate
            )
        )
        !=
        MH_OK
    )
    {
        return false;
    }


    // ========================================================
    // User32
    // ========================================================

    HMODULE hUser32 =
        GetModuleHandleA(
            "user32.dll"
        );


    if (
        !hUser32
    )
    {
        return false;
    }


    // ========================================================
    // CreateWindowExA
    // ========================================================

    pCreateWindowExA =
        reinterpret_cast<
            CreateWindowExAFn
        >(
            GetProcAddress(
                hUser32,
                "CreateWindowExA"
            )
        );


    if (
        pCreateWindowExA
    )
    {
        if (
            MH_CreateHook(
                reinterpret_cast<LPVOID>(
                    pCreateWindowExA
                ),
                reinterpret_cast<LPVOID>(
                    &MyCreateWindowExA
                ),
                reinterpret_cast<void**>(
                    &oCreateWindowExA
                )
            )
            ==
            MH_OK
        )
        {
            MH_EnableHook(
                reinterpret_cast<LPVOID>(
                    pCreateWindowExA
                )
            );
        }
    }


    // ========================================================
    // CreateWindowExW
    // ========================================================

    pCreateWindowExW =
        reinterpret_cast<
            CreateWindowExWFn
        >(
            GetProcAddress(
                hUser32,
                "CreateWindowExW"
            )
        );


    if (
        pCreateWindowExW
    )
    {
        if (
            MH_CreateHook(
                reinterpret_cast<LPVOID>(
                    pCreateWindowExW
                ),
                reinterpret_cast<LPVOID>(
                    &MyCreateWindowExW
                ),
                reinterpret_cast<void**>(
                    &oCreateWindowExW
                )
            )
            ==
            MH_OK
        )
        {
            MH_EnableHook(
                reinterpret_cast<LPVOID>(
                    pCreateWindowExW
                )
            );
        }
    }


    return true;
}


// ============================================================
// Cleanup
// ============================================================

void CleanupHooks()
{
    if (
        g_hwnd
        &&
        oWndProc
    )
    {
        SetWindowLongPtr(
            g_hwnd,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(
                oWndProc
            )
        );


        oWndProc =
            nullptr;
    }


    Menu::Uninit();


    // ========================================================
    // Remove Human Market trace
    // ========================================================

    driver.RemoveHumanMarketTrace();


    MH_DisableHook(
        MH_ALL_HOOKS
    );


    MH_Uninitialize();
}


// ============================================================
// Window procedure
// ============================================================

LRESULT __stdcall MyWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    return
        Menu::ImplWin32_WndProcHandler(
            oWndProc,
            hWnd,
            uMsg,
            wParam,
            lParam
        );
}


// ============================================================
// DllMain
// ============================================================

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
)
{
    (void)lpReserved;


    switch (
        ul_reason_for_call
    )
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(
                hModule
            );


            SetupHooks();


            break;
        }


        case DLL_PROCESS_DETACH:
        {
            CleanupHooks();


            break;
        }


        default:
            break;
    }


    return TRUE;
}
