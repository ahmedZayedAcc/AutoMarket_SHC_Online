#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct HotkeyConfig {
    UINT vkCode;
    bool ctrl;
    bool shift;
};

struct ItemConfig {
    std::string name;
    std::wstring displayName;
    int saleThreshold;
    int buyThreshold;
    bool isWeapon;
};

struct UIConfig {
    int menuWidth;
    int rowHeight;
    int offsetX; // 0 = Center
    int offsetY; // 0 = Center
    std::wstring fontName;
    int fontSize;
    int titleSize;

    COLORREF bgColor;
    COLORREF headerColor;
    COLORREF rowColor;
    COLORREF selColor;
    COLORREF editColor;
    COLORREF catColor;

    COLORREF textColor;
    COLORREF selTextColor;
    COLORREF editTextColor;
    COLORREF titleColor;
};

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Load(const std::wstring& iniPath);
    void Save(const std::wstring& iniPath);
    void ResetAll();
    void SaveSnapshot();
    void LoadSnapshot();

    HotkeyConfig GetToggleMenu() const { return m_toggleMenu; }
    HotkeyConfig GetTogglePause() const { return m_togglePause; } // تمت الإضافة هنا
    HotkeyConfig GetSaveConfig() const { return m_saveConfig; }
    HotkeyConfig GetLoadSnapshot() const { return m_loadSnapshot; }
    HotkeyConfig GetResetAll() const { return m_resetAll; }
    HotkeyConfig GetReloadConfig() const { return m_reloadConfig; }

    const std::vector<ItemConfig>& GetItems() const { return m_items; }
    void SetItemSale(int index, int value);
    void SetItemBuy(int index, int value);

    const UIConfig& GetUI() const { return m_ui; }
    int GetTradeFrequency() const { return m_tradeFrequency; }
    int GetDefaultSale() const { return m_defaultSale; }
    int GetDefaultBuy() const { return m_defaultBuy; }
    int GetWeaponCount() const { return m_weaponCount; }
    std::wstring GetIniPath() const { return m_iniPath; }

    static bool CheckHotkey(const HotkeyConfig& hk);

private:
    ConfigManager();
    void InitDefaultItems();
    int ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal, const std::wstring& path);
    std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal, const std::wstring& path);
    COLORREF ReadColor(const wchar_t* section, const wchar_t* key, COLORREF defaultColor, const std::wstring& path);
    void WriteColor(const wchar_t* section, const wchar_t* key, COLORREF color, const std::wstring& path);
    void LoadItemThresholds(const std::wstring& path);
    void SaveItemThresholds(const std::wstring& path);

    std::wstring m_iniPath;
    std::wstring m_savePath;
    
    // تمت إضافة m_togglePause في السطر التالي
    HotkeyConfig m_toggleMenu, m_togglePause, m_saveConfig, m_loadSnapshot, m_resetAll, m_reloadConfig;
    UIConfig m_ui;
    
    int m_tradeFrequency;
    int m_defaultSale, m_defaultBuy;
    int m_weaponCount;
    std::vector<ItemConfig> m_items;
};
