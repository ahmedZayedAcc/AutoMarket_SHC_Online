#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

#include <windows.h>
#include <windowsx.h>

#include "Menu.hpp"
#include "config.hpp"

namespace Menu
{
    // =========================================================
    // GDI
    // =========================================================

    HBRUSH hBgBrush = nullptr;
    HBRUSH hHeaderBrush = nullptr;
    HBRUSH hRowBrush = nullptr;
    HBRUSH hSelBrush = nullptr;
    HBRUSH hEditBrush = nullptr;
    HBRUSH hCatBrush = nullptr;
    HBRUSH hNotifBrush = nullptr;
    HBRUSH hButtonBrush = nullptr;
    HBRUSH hButtonHoverBrush = nullptr;

    HFONT hTitleFont = nullptr;
    HFONT hFont = nullptr;
    HFONT hSmallFont = nullptr;

    HDC hdc = nullptr;
    HWND g_hwnd = nullptr;

    // =========================================================
    // Surface
    // =========================================================

    int surfaceW = 0;
    int surfaceH = 0;

    // =========================================================
    // Menu position
    // =========================================================

    int offsetX = 0;
    int offsetY = 0;

    RECT menuCanvas = {};

    // =========================================================
    // State
    // =========================================================

    bool isMenuActive = false;
    bool isEditing = false;
    bool hasNotif = false;
    bool pendingClose = false;

    int selectedRow = 0;
    int selectedCol = 0;

    std::wstring editBuffer;
    std::wstring notifText;

    std::chrono::time_point<
        std::chrono::high_resolution_clock
    > notifEnd;

    // =========================================================
    // Mouse
    // =========================================================

    bool mouseCapturedByMenu = false;

    int mouseX = -1;
    int mouseY = -1;

    // =========================================================
    // Layout
    // =========================================================

    int titleH = 36;
    int headerH = 27;
    int categoryH = 24;

    int rowH = 30;

    int panelGap = 12;

    int saveH = 34;

    // =========================================================
    // Helpers
    // =========================================================

    void DestroyBrush(
        HBRUSH& b
    )
    {
        if (b)
        {
            ::DeleteObject(b);
            b = nullptr;
        }
    }

    void DestroyFont(
        HFONT& f
    )
    {
        if (f)
        {
            ::DeleteObject(f);
            f = nullptr;
        }
    }

    // =========================================================
    // Reload UI
    // =========================================================

    void ReloadUI()
    {
        auto& ui =
            ConfigManager::Instance().GetUI();

        DestroyBrush(hBgBrush);
        DestroyBrush(hHeaderBrush);
        DestroyBrush(hRowBrush);
        DestroyBrush(hSelBrush);
        DestroyBrush(hEditBrush);
        DestroyBrush(hCatBrush);
        DestroyBrush(hNotifBrush);
        DestroyBrush(hButtonBrush);
        DestroyBrush(hButtonHoverBrush);

        DestroyFont(hTitleFont);
        DestroyFont(hFont);
        DestroyFont(hSmallFont);

        hBgBrush =
            CreateSolidBrush(
                ui.bgColor
            );

        hHeaderBrush =
            CreateSolidBrush(
                ui.headerColor
            );

        hRowBrush =
            CreateSolidBrush(
                ui.rowColor
            );

        hSelBrush =
            CreateSolidBrush(
                ui.selColor
            );

        hEditBrush =
            CreateSolidBrush(
                ui.editColor
            );

        hCatBrush =
            CreateSolidBrush(
                ui.catColor
            );

        hNotifBrush =
            CreateSolidBrush(
                RGB(40, 80, 40)
            );

        hButtonBrush =
            CreateSolidBrush(
                RGB(70, 70, 70)
            );

        hButtonHoverBrush =
            CreateSolidBrush(
                RGB(95, 95, 95)
            );

        hTitleFont =
            CreateFontW(
                ui.titleSize,
                0,
                0,
                0,
                FW_BOLD,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                FIXED_PITCH,
                ui.fontName.c_str()
            );

        hFont =
            CreateFontW(
                ui.fontSize,
                0,
                0,
                0,
                FW_NORMAL,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                FIXED_PITCH,
                ui.fontName.c_str()
            );

        hSmallFont =
            CreateFontW(
                std::max(
                    8,
                    ui.fontSize - 2
                ),
                0,
                0,
                0,
                FW_NORMAL,
                FALSE,
                FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                FIXED_PITCH,
                ui.fontName.c_str()
            );

        // -----------------------------------------------------
        // Row height follows original UI setting
        // -----------------------------------------------------

        rowH =
            std::max(
                26,
                ui.rowHeight
            );
    }

    // =========================================================
    // Init
    // =========================================================

    void Init(HWND hwnd)
    {
        g_hwnd = hwnd;

        surfaceW = 0;
        surfaceH = 0;

        isMenuActive = false;
        isEditing = false;
        hasNotif = false;
        pendingClose = false;

        selectedRow = 0;
        selectedCol = 0;

        mouseCapturedByMenu = false;

        mouseX = -1;
        mouseY = -1;

        ReloadUI();
    }

    // =========================================================
    // Uninit
    // =========================================================

    void Uninit()
    {
        DestroyBrush(hBgBrush);
        DestroyBrush(hHeaderBrush);
        DestroyBrush(hRowBrush);
        DestroyBrush(hSelBrush);
        DestroyBrush(hEditBrush);
        DestroyBrush(hCatBrush);
        DestroyBrush(hNotifBrush);
        DestroyBrush(hButtonBrush);
        DestroyBrush(hButtonHoverBrush);

        DestroyFont(hTitleFont);
        DestroyFont(hFont);
        DestroyFont(hSmallFont);

        hdc = nullptr;
        g_hwnd = nullptr;
    }

    // =========================================================
    // Surface size
    // =========================================================

    void SetSurfaceSize(
        int width,
        int height
    )
    {
        if (width > 0)
            surfaceW = width;

        if (height > 0)
            surfaceH = height;
    }

    // =========================================================
    // Fill rect
    // =========================================================

    void FR(
        RECT r,
        HBRUSH brush
    )
    {
        OffsetRect(
            &r,
            offsetX,
            offsetY
        );

        FillRect(
            hdc,
            &r,
            brush
        );
    }

    // =========================================================
    // Text
    // =========================================================

    void DT(
        RECT r,
        HFONT font,
        const wchar_t* text,
        int format,
        COLORREF color
    )
    {
        OffsetRect(
            &r,
            offsetX,
            offsetY
        );

        HFONT old =
            (HFONT)SelectObject(
                hdc,
                font
            );

        SetTextColor(
            hdc,
            color
        );

        SetBkMode(
            hdc,
            TRANSPARENT
        );

        DrawTextW(
            hdc,
            text,
            -1,
            &r,
            format |
            DT_NOPREFIX
        );

        SelectObject(
            hdc,
            old
        );
    }

    // =========================================================
    // Notification
    // =========================================================

    void ShowNotif(
        const std::wstring& text,
        int ms
    )
    {
        notifText = text;

        notifEnd =
            std::chrono::high_resolution_clock::now()
            +
            std::chrono::milliseconds(ms);

        hasNotif = true;
    }

    void ProcessNotif()
    {
        if (!hasNotif)
            return;

        if (
            std::chrono::high_resolution_clock::now()
            >= notifEnd
        )
        {
            hasNotif = false;

            if (pendingClose)
            {
                isMenuActive = false;
                isEditing = false;
                pendingClose = false;
                mouseCapturedByMenu = false;
            }
        }
    }

    // =========================================================
    // Apply edit
    // =========================================================

    void ApplyEdit()
    {
        if (!isEditing)
            return;

        auto& cfg =
            ConfigManager::Instance();

        int value = 0;

        if (!editBuffer.empty())
        {
            try
            {
                value =
                    std::stoi(
                        editBuffer
                    );
            }
            catch (...)
            {
                value = 0;
            }
        }

        value =
            std::max(
                0,
                std::min(
                    9999,
                    value
                )
            );

        if (
            selectedRow >= 0 &&
            selectedRow <
                static_cast<int>(
                    cfg.GetItems().size()
                )
        )
        {
            if (selectedCol == 0)
            {
                cfg.SetItemSale(
                    selectedRow,
                    value
                );
            }
            else
            {
                cfg.SetItemBuy(
                    selectedRow,
                    value
                );
            }
        }

        isEditing = false;
        editBuffer.clear();
    }

    // =========================================================
    // Save INI
    // =========================================================

    void SaveConfig()
    {
        ApplyEdit();

        auto& cfg =
            ConfigManager::Instance();

        cfg.Save(
            cfg.GetIniPath()
        );

        cfg.SaveSnapshot();

        ShowNotif(
            L"Settings Saved",
            1400
        );
    }

    // =========================================================
    // Calculate panel heights
    // =========================================================

    int GetPanelHeight(
        int count
    )
    {
        if (count <= 0)
            return 0;

        return
            categoryH +
            headerH +
            count * rowH;
    }

    // =========================================================
    // Menu height
    // =========================================================

    int GetMenuHeight()
    {
        auto& cfg =
            ConfigManager::Instance();

        int wc =
            cfg.GetWeaponCount();

        int total =
            static_cast<int>(
                cfg.GetItems().size()
            );

        int rc =
            std::max(
                0,
                total - wc
            );

        int weaponH =
            GetPanelHeight(wc);

        int resourceH =
            GetPanelHeight(rc);

        int contentH =
            std::max(
                weaponH,
                resourceH
            );

        return
            titleH +
            contentH +
            saveH;
    }

    // =========================================================
    // Menu width
    // =========================================================

    int GetMenuWidth()
    {
        auto& ui =
            ConfigManager::Instance().GetUI();

        return ui.menuWidth;
    }

    // =========================================================
    // Menu rectangle
    // =========================================================

    RECT GetMenuRect()
    {
        int w =
            GetMenuWidth();

        int h =
            GetMenuHeight();

        return {
            offsetX,
            offsetY,
            offsetX + w,
            offsetY + h
        };
    }

    // =========================================================
    // Point inside menu
    // =========================================================

    bool IsPointInsideMenu(
        int x,
        int y
    )
    {
        RECT r =
            GetMenuRect();

        return
            x >= r.left &&
            x < r.right &&
            y >= r.top &&
            y < r.bottom;
    }

    // =========================================================
    // Convert Window coordinates to Surface coordinates
    // =========================================================

    POINT WindowToSurface(
        int x,
        int y
    )
    {
        POINT p = {
            x,
            y
        };

        RECT client = {};

        if (
            g_hwnd &&
            GetClientRect(
                g_hwnd,
                &client
            )
        )
        {
            int clientW =
                client.right -
                client.left;

            int clientH =
                client.bottom -
                client.top;

            if (
                clientW > 0 &&
                clientH > 0 &&
                surfaceW > 0 &&
                surfaceH > 0
            )
            {
                p.x =
                    static_cast<LONG>(
                        (
                            static_cast<long long>(x)
                            *
                            surfaceW
                        )
                        /
                        clientW
                    );

                p.y =
                    static_cast<LONG>(
                        (
                            static_cast<long long>(y)
                            *
                            surfaceH
                        )
                        /
                        clientH
                    );
            }
        }

        return p;
    }

    // =========================================================
    // Save button
    // =========================================================

    RECT GetSaveRect()
    {
        int w =
            GetMenuWidth();

        int h =
            GetMenuHeight();

        int buttonW = 190;

        return {
            (w - buttonW) / 2,
            h - saveH + 5,
            (w + buttonW) / 2,
            h - 5
        };
    }

    // =========================================================
    // Draw row
    // =========================================================

    void DrawRow(
        int index,
        int cy,
        int x,
        int width,
        const std::wstring& name,
        int sell,
        int buy,
        const UIConfig& ui
    )
    {
        bool selected =
            index == selectedRow;

        FR(
            {
                x,
                cy,
                x + width,
                cy + rowH
            },
            selected
            ? hSelBrush
            : hRowBrush
        );

        int nameW =
            static_cast<int>(
                width * 0.45
            );

        int sellW =
            static_cast<int>(
                width * 0.25
            );

        int buyW =
            width -
            nameW -
            sellW;

        (void)buyW;

        std::wstring prefix =
            selected
            ? L"> "
            : L"  ";

        std::wstring itemName =
            prefix +
            name;

        DT(
            {
                x + 5,
                cy,
                x + nameW,
                cy + rowH
            },
            hFont,
            itemName.c_str(),
            DT_LEFT |
            DT_VCENTER |
            DT_SINGLELINE |
            DT_END_ELLIPSIS,
            selected
            ? ui.selTextColor
            : ui.textColor
        );

        bool sellSelected =
            selected &&
            selectedCol == 0;

        RECT sellRect = {
            x + nameW,
            cy,
            x + nameW + sellW,
            cy + rowH
        };

        if (sellSelected)
        {
            FR(
                sellRect,
                isEditing
                ? hEditBrush
                : hSelBrush
            );
        }

        std::wstring sellText;

        if (
            sellSelected &&
            isEditing
        )
        {
            sellText =
                L"[" +
                editBuffer +
                L"_]";
        }
        else
        {
            sellText =
                std::to_wstring(
                    sell
                );
        }

        DT(
            sellRect,
            hFont,
            sellText.c_str(),
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            sellSelected
            ? ui.editTextColor
            : ui.textColor
        );

        bool buySelected =
            selected &&
            selectedCol == 1;

        RECT buyRect = {
            x + nameW + sellW,
            cy,
            x + width,
            cy + rowH
        };

        if (buySelected)
        {
            FR(
                buyRect,
                isEditing
                ? hEditBrush
                : hSelBrush
            );
        }

        std::wstring buyText;

        if (
            buySelected &&
            isEditing
        )
        {
            buyText =
                L"[" +
                editBuffer +
                L"_]";
        }
        else
        {
            buyText =
                std::to_wstring(
                    buy
                );
        }

        DT(
            buyRect,
            hFont,
            buyText.c_str(),
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            buySelected
            ? ui.editTextColor
            : ui.textColor
        );
    }

    // =========================================================
    // Draw panel
    // =========================================================

    void DrawPanel(
        int x,
        int y,
        int width,
        int startIndex,
        int count,
        const wchar_t* title,
        const UIConfig& ui
    )
    {
        if (count <= 0)
            return;

        FR(
            {
                x,
                y,
                x + width,
                y + categoryH
            },
            hCatBrush
        );

        DT(
            {
                x + 7,
                y,
                x + width - 7,
                y + categoryH
            },
            hFont,
            title,
            DT_LEFT |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.textColor
        );

        y += categoryH;

        FR(
            {
                x,
                y,
                x + width,
                y + headerH
            },
            hHeaderBrush
        );

        int nameW =
            static_cast<int>(
                width * 0.45
            );

        int sellW =
            static_cast<int>(
                width * 0.25
            );

        DT(
            {
                x + 5,
                y,
                x + nameW,
                y + headerH
            },
            hFont,
            L"Item",
            DT_LEFT |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.textColor
        );

        DT(
            {
                x + nameW,
                y,
                x + nameW + sellW,
                y + headerH
            },
            hFont,
            L"Sell >",
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.textColor
        );

        DT(
            {
                x + nameW + sellW,
                y,
                x + width,
                y + headerH
            },
            hFont,
            L"Buy <",
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.textColor
        );

        y += headerH;

        auto& cfg =
            ConfigManager::Instance();

        const auto& items =
            cfg.GetItems();

        for (int i = 0; i < count; i++)
        {
            int index =
                startIndex + i;

            if (
                index < 0 ||
                index >=
                    static_cast<int>(
                        items.size()
                    )
            )
            {
                break;
            }

            DrawRow(
                index,
                y,
                x,
                width,
                items[index].displayName,
                items[index].saleThreshold,
                items[index].buyThreshold,
                ui
            );

            y += rowH;
        }
    }

    // =========================================================
    // Hit test panel
    // =========================================================

    bool HitPanel(
        int mx,
        int my,
        int x,
        int y,
        int width,
        int startIndex,
        int count,
        int& index,
        int& column
    )
    {
        if (count <= 0)
            return false;

        y += categoryH;
        y += headerH;

        if (
            mx < x ||
            mx >= x + width
        )
        {
            return false;
        }

        if (my < y)
            return false;

        int row =
            (my - y) /
            rowH;

        if (
            row < 0 ||
            row >= count
        )
        {
            return false;
        }

        index =
            startIndex + row;

        int localX =
            mx - x;

        int nameW =
            static_cast<int>(
                width * 0.45
            );

        int sellW =
            static_cast<int>(
                width * 0.25
            );

        if (
            localX < nameW
        )
        {
            column = selectedCol;
        }
        else if (
            localX <
            nameW + sellW
        )
        {
            column = 0;
        }
        else
        {
            column = 1;
        }

        return true;
    }

    // =========================================================
    // Mouse click
    // =========================================================

    bool HandleMouseClick(
        int x,
        int y
    )
    {
        if (!isMenuActive)
            return false;

        POINT p =
            WindowToSurface(
                x,
                y
            );

        int sx =
            static_cast<int>(
                p.x
            );

        int sy =
            static_cast<int>(
                p.y
            );

        if (
            !IsPointInsideMenu(
                sx,
                sy
            )
        )
        {
            return false;
        }

        int mx =
            sx - offsetX;

        int my =
            sy - offsetY;

        // -----------------------------------------------------
        // Save button
        // -----------------------------------------------------

        RECT save =
            GetSaveRect();

        if (
            mx >= save.left &&
            mx < save.right &&
            my >= save.top &&
            my < save.bottom
        )
        {
            SaveConfig();

            return true;
        }

        // -----------------------------------------------------
        // Panels
        // -----------------------------------------------------

        auto& cfg =
            ConfigManager::Instance();

        int wc =
            cfg.GetWeaponCount();

        int total =
            static_cast<int>(
                cfg.GetItems().size()
            );

        int rc =
            std::max(
                0,
                total - wc
            );

        int contentWidth =
            GetMenuWidth();

        int panelWidth =
            (
                contentWidth -
                panelGap
            ) / 2;

        int leftX = 0;

        int rightX =
            panelWidth +
            panelGap;

        int index = -1;
        int column = 0;

        if (wc > 0)
        {
            if (
                HitPanel(
                    mx,
                    my,
                    leftX,
                    titleH,
                    panelWidth,
                    0,
                    wc,
                    index,
                    column
                )
            )
            {
                selectedRow = index;
                selectedCol = column;

                const auto& item =
                    cfg.GetItems()[index];

                int value =
                    selectedCol == 0
                    ? item.saleThreshold
                    : item.buyThreshold;

                isEditing = true;

                editBuffer =
                    std::to_wstring(
                        value
                    );

                return true;
            }
        }

        if (rc > 0)
        {
            if (
                HitPanel(
                    mx,
                    my,
                    rightX,
                    titleH,
                    panelWidth,
                    wc,
                    rc,
                    index,
                    column
                )
            )
            {
                selectedRow = index;
                selectedCol = column;

                const auto& item =
                    cfg.GetItems()[index];

                int value =
                    selectedCol == 0
                    ? item.saleThreshold
                    : item.buyThreshold;

                isEditing = true;

                editBuffer =
                    std::to_wstring(
                        value
                    );

                return true;
            }
        }

        return true;
    }

    // =========================================================
    // WndProc
    // =========================================================

    LRESULT ImplWin32_WndProcHandler(
        WNDPROC oldWndProc,
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        auto& cfg =
            ConfigManager::Instance();

        // =====================================================
        // Mouse move
        // =====================================================

        if (uMsg == WM_MOUSEMOVE)
        {
            mouseX =
                GET_X_LPARAM(
                    lParam
                );

            mouseY =
                GET_Y_LPARAM(
                    lParam
                );

            return CallWindowProc(
                oldWndProc,
                hWnd,
                uMsg,
                wParam,
                lParam
            );
        }

        // =====================================================
        // Mouse down
        // =====================================================

        if (uMsg == WM_LBUTTONDOWN)
        {
            int x =
                GET_X_LPARAM(
                    lParam
                );

            int y =
                GET_Y_LPARAM(
                    lParam
                );

            POINT p =
                WindowToSurface(
                    x,
                    y
                );

            if (
                isMenuActive &&
                IsPointInsideMenu(
                    static_cast<int>(p.x),
                    static_cast<int>(p.y)
                )
            )
            {
                mouseCapturedByMenu = true;

                HandleMouseClick(
                    x,
                    y
                );

                return 0;
            }

            return CallWindowProc(
                oldWndProc,
                hWnd,
                uMsg,
                wParam,
                lParam
            );
        }

        // =====================================================
        // Mouse up
        // =====================================================

        if (uMsg == WM_LBUTTONUP)
        {
            if (mouseCapturedByMenu)
            {
                mouseCapturedByMenu = false;
                return 0;
            }

            return CallWindowProc(
                oldWndProc,
                hWnd,
                uMsg,
                wParam,
                lParam
            );
        }

        // =====================================================
        // Keyboard
        // =====================================================

        if (uMsg == WM_KEYDOWN)
        {
            if (
                lParam &
                0x40000000
            )
            {
                return CallWindowProc(
                    oldWndProc,
                    hWnd,
                    uMsg,
                    wParam,
                    lParam
                );
            }

            // -------------------------------------------------
            // Toggle
            // -------------------------------------------------

            if (
                ConfigManager::CheckHotkey(
                    cfg.GetToggleMenu()
                )
            )
            {
                if (isEditing)
                {
                    isEditing = false;
                    editBuffer.clear();
                }

                isMenuActive =
                    !isMenuActive;

                mouseCapturedByMenu = false;

                return 0;
            }

            // -------------------------------------------------
            // Reload
            // -------------------------------------------------

            if (
                ConfigManager::CheckHotkey(
                    cfg.GetReloadConfig()
                )
            )
            {
                cfg.Load(
                    cfg.GetIniPath()
                );

                ReloadUI();

                if (isMenuActive)
                {
                    ShowNotif(
                        L"Config Reloaded!",
                        1200
                    );
                }

                return 0;
            }

            // -------------------------------------------------
            // Load
            // -------------------------------------------------

            if (
                ConfigManager::CheckHotkey(
                    cfg.GetLoadSnapshot()
                )
            )
            {
                cfg.LoadSnapshot();

                if (isMenuActive)
                {
                    ShowNotif(
                        L"Loaded!",
                        1200
                    );
                }

                return 0;
            }

            if (!isMenuActive)
            {
                return CallWindowProc(
                    oldWndProc,
                    hWnd,
                    uMsg,
                    wParam,
                    lParam
                );
            }

            // -------------------------------------------------
            // Save
            // -------------------------------------------------

            if (
                ConfigManager::CheckHotkey(
                    cfg.GetSaveConfig()
                )
            )
            {
                SaveConfig();

                pendingClose = true;

                return 0;
            }

            int total =
                static_cast<int>(
                    cfg.GetItems().size()
                );

            // -------------------------------------------------
            // Editing
            // -------------------------------------------------

            if (isEditing)
            {
                if (
                    wParam >= '0' &&
                    wParam <= '9'
                )
                {
                    if (
                        editBuffer.length()
                        < 4
                    )
                    {
                        editBuffer +=
                            static_cast<wchar_t>(
                                wParam
                            );
                    }

                    return 0;
                }

                if (wParam == VK_BACK)
                {
                    if (!editBuffer.empty())
                        editBuffer.pop_back();

                    return 0;
                }

                if (wParam == VK_DELETE)
                {
                    editBuffer.clear();
                    return 0;
                }

                if (wParam == VK_RETURN)
                {
                    ApplyEdit();
                    return 0;
                }

                if (wParam == VK_ESCAPE)
                {
                    isEditing = false;
                    editBuffer.clear();
                    return 0;
                }

                if (wParam == VK_TAB)
                {
                    ApplyEdit();

                    selectedCol =
                        1 - selectedCol;

                    return 0;
                }
            }
            else
            {
                if (wParam == VK_UP)
                {
                    if (selectedRow > 0)
                        selectedRow--;

                    return 0;
                }

                if (wParam == VK_DOWN)
                {
                    if (
                        selectedRow <
                        total - 1
                    )
                    {
                        selectedRow++;
                    }

                    return 0;
                }

                if (wParam == VK_TAB)
                {
                    selectedCol =
                        1 - selectedCol;

                    return 0;
                }

                if (
                    wParam >= '0' &&
                    wParam <= '9'
                )
                {
                    if (
                        selectedRow >= 0 &&
                        selectedRow < total
                    )
                    {
                        isEditing = true;

                        editBuffer =
                            static_cast<wchar_t>(
                                wParam
                            );
                    }

                    return 0;
                }

                if (wParam == VK_ESCAPE)
                {
                    isMenuActive = false;
                    return 0;
                }
            }
        }

        return CallWindowProc(
            oldWndProc,
            hWnd,
            uMsg,
            wParam,
            lParam
        );
    }

    // =========================================================
    // Draw
    // =========================================================

    void Draw()
    {
        ProcessNotif();

        if (
            !isMenuActive &&
            !hasNotif
        )
        {
            return;
        }

        if (!hdc)
            return;

        auto& cfg =
            ConfigManager::Instance();

        auto& ui =
            cfg.GetUI();

        int wc =
            cfg.GetWeaponCount();

        int total =
            static_cast<int>(
                cfg.GetItems().size()
            );

        int rc =
            std::max(
                0,
                total - wc
            );

        int menuW =
            GetMenuWidth();

        int menuH =
            GetMenuHeight();

        // =====================================================
        // Use actual DirectDraw Surface dimensions
        // =====================================================

        if (surfaceW <= 0)
            surfaceW = menuW;

        if (surfaceH <= 0)
            surfaceH = menuH;

        // =====================================================
        // TOP CENTER
        // =====================================================

        offsetX =
            (
                surfaceW -
                menuW
            ) / 2;

        offsetY =
            ui.offsetY;

        offsetX +=
            ui.offsetX;

        if (offsetX < 0)
            offsetX = 0;

        if (offsetY < 0)
            offsetY = 0;

        if (
            offsetY + menuH >
            surfaceH
        )
        {
            offsetY = 0;
        }

        menuCanvas = {
            0,
            0,
            menuW,
            menuH
        };

        // =====================================================
        // Background
        // =====================================================

        FR(
            menuCanvas,
            hBgBrush
        );

        // =====================================================
        // Title
        // =====================================================

        DT(
            {
                0,
                0,
                menuW,
                titleH
            },
            hTitleFont,
            L"AutoMarket By Ahmed Zayed",
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.titleColor
        );

        // =====================================================
        // Panels
        // =====================================================

        int panelWidth =
            (
                menuW -
                panelGap
            ) / 2;

        int leftX = 0;

        int rightX =
            panelWidth +
            panelGap;

        DrawPanel(
            leftX,
            titleH,
            panelWidth,
            0,
            wc,
            L"-- Weapons --",
            ui
        );

        DrawPanel(
            rightX,
            titleH,
            panelWidth,
            wc,
            rc,
            L"-- Resources --",
            ui
        );

        // =====================================================
        // Save button
        // =====================================================

        RECT save =
            GetSaveRect();

        POINT mouseSurface =
            WindowToSurface(
                mouseX,
                mouseY
            );

        bool hover =
            mouseX >= 0 &&
            mouseY >= 0 &&
            mouseSurface.x >= offsetX + save.left &&
            mouseSurface.x < offsetX + save.right &&
            mouseSurface.y >= offsetY + save.top &&
            mouseSurface.y < offsetY + save.bottom;

        FR(
            save,
            hover
            ? hButtonHoverBrush
            : hButtonBrush
        );

        DT(
            save,
            hFont,
            L"SAVE SETTINGS",
            DT_CENTER |
            DT_VCENTER |
            DT_SINGLELINE,
            ui.textColor
        );

        // =====================================================
        // Notification
        // =====================================================

        if (hasNotif)
        {
            RECT nr = {
                menuW / 4,
                menuH / 2 - 16,
                menuW * 3 / 4,
                menuH / 2 + 16
            };

            FR(
                nr,
                hNotifBrush
            );

            DT(
                nr,
                hFont,
                notifText.c_str(),
                DT_CENTER |
                DT_VCENTER |
                DT_SINGLELINE,
                RGB(
                    200,
                    255,
                    200
                )
            );
        }
    }

    // =========================================================
    // Context
    // =========================================================

    void SetContext(
        HDC newHdc
    )
    {
        hdc = newHdc;
    }

    // =========================================================
    // Visible
    // =========================================================

    bool IsVisible()
    {
        return
            isMenuActive ||
            hasNotif;
    }
}
