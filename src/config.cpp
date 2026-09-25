#include "config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : m_defaultSale(500), m_defaultBuy(0), m_weaponCount(0), m_tradeFrequency(50) {
    m_toggleMenu    = { 0x61, false, false }; // Numpad 1
    m_togglePause   = { 0x62, false, false }; // Numpad 2
    m_saveConfig    = { 0x53, true,  true  };
    m_loadSnapshot  = { 0x4C, true,  true  };
    m_resetAll      = { 0x52, true,  false };
    m_reloadConfig  = { 0x4C, true,  false };
    InitDefaultItems();
}

void ConfigManager::InitDefaultItems() {
    m_items.clear();
    m_items.push_back({"bows",          L"Bows",          m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"crossbows",     L"Crossbows",     m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"leather armor", L"Leather Armor", m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"maces",         L"Maces",         m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"metal armor",   L"Metal Armor",   m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"pikes",         L"Pikes",         m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"spears",        L"Spears",        m_defaultSale, m_defaultBuy, true});
    m_items.push_back({"swords",        L"Swords",        m_defaultSale, m_defaultBuy, true});
    m_weaponCount = (int)m_items.size();
    m_items.push_back({"ale",    L"Ale (Beer)",     m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"bread",  L"Bread",          m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"cheese", L"Cheese",         m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"flour",  L"Flour",          m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"fruit",  L"Fruit (Apples)", m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"hops",   L"Hops",           m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"iron",   L"Iron",           m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"meat",   L"Meat",           m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"pitch",  L"Pitch",          m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"stone",  L"Stone",          m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"wheat",  L"Wheat",          m_defaultSale, m_defaultBuy, false});
    m_items.push_back({"wood",   L"Wood",           m_defaultSale, m_defaultBuy, false});
}

int ConfigManager::ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal, const std::wstring& path) {
    return GetPrivateProfileIntW(section, key, defaultVal, path.c_str());
}

std::wstring ConfigManager::ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal, const std::wstring& path) {
    wchar_t buf[256];
    GetPrivateProfileStringW(section, key, defaultVal, buf, 256, path.c_str());
    return std::wstring(buf);
}

COLORREF ConfigManager::ReadColor(const wchar_t* section, const wchar_t* key, COLORREF defaultColor, const std::wstring& path) {
    std::wstring s = ReadString(section, key, L"", path);
    if (s.empty()) return defaultColor;
    int r = 0, g = 0, b = 0;
    if (swscanf(s.c_str(), L"%d,%d,%d", &r, &g, &b) == 3) {
        return RGB(r, g, b);
    }
    return defaultColor;
}

void ConfigManager::WriteColor(const wchar_t* section, const wchar_t* key, COLORREF color, const std::wstring& path) {
    std::wstring s = std::to_wstring(GetRValue(color)) + L"," + std::to_wstring(GetGValue(color)) + L"," + std::to_wstring(GetBValue(color));
    WritePrivateProfileStringW(section, key, s.c_str(), path.c_str());
}

void ConfigManager::LoadItemThresholds(const std::wstring& path) {
    const wchar_t* weaponKeys[] = {L"Bows", L"Crossbows", L"LeatherArmor", L"Maces", L"MetalArmor", L"Pikes", L"Spears", L"Swords"};
    for (int i = 0; i < m_weaponCount; i++) {
        std::wstring val = ReadString(L"Weapons", weaponKeys[i], L"", path);
        if (!val.empty()) {
            swscanf(val.c_str(), L"%d,%d", &m_items[i].saleThreshold, &m_items[i].buyThreshold);
        }
    }
    const wchar_t* resKeys[] = {L"Ale", L"Bread", L"Cheese", L"Flour", L"Fruit", L"Hops", L"Iron", L"Meat", L"Pitch", L"Stone", L"Wheat", L"Wood"};
    int resCount = (int)m_items.size() - m_weaponCount;
    for (int i = 0; i < resCount; i++) {
        std::wstring val = ReadString(L"Resources", resKeys[i], L"", path);
        if (!val.empty()) {
            swscanf(val.c_str(), L"%d,%d", &m_items[m_weaponCount+i].saleThreshold, &m_items[m_weaponCount+i].buyThreshold);
        }
    }
}

void ConfigManager::SaveItemThresholds(const std::wstring& path) {
    const wchar_t* weaponKeys[] = {L"Bows", L"Crossbows", L"LeatherArmor", L"Maces", L"MetalArmor", L"Pikes", L"Spears", L"Swords"};
    for (int i = 0; i < m_weaponCount; i++) {
        std::wstring val = std::to_wstring(m_items[i].saleThreshold) + L"," + std::to_wstring(m_items[i].buyThreshold);
        WritePrivateProfileStringW(L"Weapons", weaponKeys[i], val.c_str(), path.c_str());
    }
    const wchar_t* resKeys[] = {L"Ale", L"Bread", L"Cheese", L"Flour", L"Fruit", L"Hops", L"Iron", L"Meat", L"Pitch", L"Stone", L"Wheat", L"Wood"};
    int resCount = (int)m_items.size() - m_weaponCount;
    for (int i = 0; i < resCount; i++) {
        std::wstring val = std::to_wstring(m_items[m_weaponCount+i].saleThreshold) + L"," + std::to_wstring(m_items[m_weaponCount+i].buyThreshold);
        WritePrivateProfileStringW(L"Resources", resKeys[i], val.c_str(), path.c_str());
    }
}

void ConfigManager::Load(const std::wstring& iniPath) {
    m_iniPath = iniPath;
    wchar_t dir[MAX_PATH];
    lstrcpynW(dir, iniPath.c_str(), MAX_PATH);
    if (wchar_t* lastSlash = wcsrchr(dir, L'\\')) *(lastSlash + 1) = L'\0';
    m_savePath = std::wstring(dir) + L"automarketsave.ini";

    // Hotkeys
    m_toggleMenu.vkCode = ReadInt(L"Hotkeys", L"ToggleMenu", 0x61, m_iniPath); 
    m_toggleMenu.ctrl = ReadInt(L"Hotkeys", L"ToggleMenu_Ctrl", 0, m_iniPath);
    m_toggleMenu.shift = ReadInt(L"Hotkeys", L"ToggleMenu_Shift", 0, m_iniPath);

    m_togglePause.vkCode = ReadInt(L"Hotkeys", L"TogglePause", 0x62, m_iniPath); 
    m_togglePause.ctrl = ReadInt(L"Hotkeys", L"TogglePause_Ctrl", 0, m_iniPath);
    m_togglePause.shift = ReadInt(L"Hotkeys", L"TogglePause_Shift", 0, m_iniPath);

    m_saveConfig.vkCode = ReadInt(L"Hotkeys", L"SaveConfig", 0x53, m_iniPath);
    m_saveConfig.ctrl = ReadInt(L"Hotkeys", L"SaveConfig_Ctrl", 1, m_iniPath);
    m_saveConfig.shift = ReadInt(L"Hotkeys", L"SaveConfig_Shift", 1, m_iniPath);

    m_loadSnapshot.vkCode = ReadInt(L"Hotkeys", L"LoadSnapshot", 0x4C, m_iniPath);
    m_loadSnapshot.ctrl = ReadInt(L"Hotkeys", L"LoadSnapshot_Ctrl", 1, m_iniPath);
    m_loadSnapshot.shift = ReadInt(L"Hotkeys", L"LoadSnapshot_Shift", 1, m_iniPath);

    m_resetAll.vkCode = ReadInt(L"Hotkeys", L"ResetAll", 0x52, m_iniPath);
    m_resetAll.ctrl = ReadInt(L"Hotkeys", L"ResetAll_Ctrl", 1, m_iniPath);
    m_resetAll.shift = ReadInt(L"Hotkeys", L"ResetAll_Shift", 0, m_iniPath);

    m_reloadConfig.vkCode = ReadInt(L"Hotkeys", L"ReloadConfig", 0x4C, m_iniPath);
    m_reloadConfig.ctrl = ReadInt(L"Hotkeys", L"ReloadConfig_Ctrl", 1, m_iniPath);
    m_reloadConfig.shift = ReadInt(L"Hotkeys", L"ReloadConfig_Shift", 0, m_iniPath);

    // Advanced
    m_tradeFrequency = ReadInt(L"Advanced", L"TradeFrequencyMs", 50, m_iniPath);

    // UI
    m_ui.menuWidth = ReadInt(L"UI", L"MenuWidth", 400, m_iniPath);
    m_ui.rowHeight = ReadInt(L"UI", L"RowHeight", 22, m_iniPath);
    m_ui.offsetX = ReadInt(L"UI", L"OffsetX", 0, m_iniPath);
    m_ui.offsetY = ReadInt(L"UI", L"OffsetY", 0, m_iniPath);
    m_ui.fontName = ReadString(L"UI", L"FontName", L"Consolas", m_iniPath);
    m_ui.fontSize = ReadInt(L"UI", L"FontSize", 16, m_iniPath);
    m_ui.titleSize = ReadInt(L"UI", L"TitleSize", 20, m_iniPath);

    // --- درجات الألوان المأخوذة من صورة الواجهة مباشرة ---
    m_ui.bgColor     = ReadColor(L"UI", L"BgColor", RGB(246, 224, 175), m_iniPath);       // بيج رملي (خلفية)
    m_ui.headerColor = ReadColor(L"UI", L"HeaderColor", RGB(198, 126, 88), m_iniPath);   // بني نحاسي للرأس والزر السفلي
    m_ui.rowColor    = ReadColor(L"UI", L"RowColor", RGB(250, 232, 192), m_iniPath);      // بيج فاتح للصفوف
    m_ui.selColor    = ReadColor(L"UI", L"SelColor", RGB(175, 105, 70), m_iniPath);     // بني غامق عند التحديد
    m_ui.editColor   = ReadColor(L"UI", L"EditColor", RGB(198, 126, 88), m_iniPath);    // بني للأزرار (+ و -)
    m_ui.catColor    = ReadColor(L"UI", L"CatColor", RGB(225, 168, 112), m_iniPath);      // ذهبي بني لشريط الفئات (Weapons / Resources)

    m_ui.textColor     = ReadColor(L"UI", L"TextColor", RGB(35, 25, 15), m_iniPath);        // بني داكن/أسود لأسماء المواد والأرقام
    m_ui.selTextColor  = ReadColor(L"UI", L"HighlightTextColor", RGB(255, 255, 255), m_iniPath); // أبيض للأزرار والتحديد
    m_ui.editTextColor = ReadColor(L"UI", L"EditTextColor", RGB(255, 255, 255), m_iniPath);    // أبيض لنصوص أزرار + و -
    m_ui.titleColor    = ReadColor(L"UI", L"TitleColor", RGB(35, 25, 15), m_iniPath);       // بني داكن للعنوان الرئيسي

    m_defaultSale = ReadInt(L"Defaults", L"DefaultSale", 500, m_iniPath);
    m_defaultBuy  = ReadInt(L"Defaults", L"DefaultBuy", 0, m_iniPath);

    InitDefaultItems();
    LoadItemThresholds(m_iniPath);

    DWORD attr = GetFileAttributesW(m_savePath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        LoadItemThresholds(m_savePath);
    }
}

void ConfigManager::Save(const std::wstring& iniPath) {
    if (iniPath.empty()) return;

    WritePrivateProfileStringW(L"Advanced", L"TradeFrequencyMs", std::to_wstring(m_tradeFrequency).c_str(), iniPath.c_str());

    WritePrivateProfileStringW(L"UI", L"MenuWidth", std::to_wstring(m_ui.menuWidth).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"RowHeight", std::to_wstring(m_ui.rowHeight).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"OffsetX", std::to_wstring(m_ui.offsetX).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"OffsetY", std::to_wstring(m_ui.offsetY).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"FontName", m_ui.fontName.c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"FontSize", std::to_wstring(m_ui.fontSize).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(L"UI", L"TitleSize", std::to_wstring(m_ui.titleSize).c_str(), iniPath.c_str());

    WriteColor(L"UI", L"BgColor", m_ui.bgColor, iniPath);
    WriteColor(L"UI", L"HeaderColor", m_ui.headerColor, iniPath);
    WriteColor(L"UI", L"RowColor", m_ui.rowColor, iniPath);
    WriteColor(L"UI", L"SelColor", m_ui.selColor, iniPath);
    WriteColor(L"UI", L"EditColor", m_ui.editColor, iniPath);
    WriteColor(L"UI", L"CatColor", m_ui.catColor, iniPath);
    WriteColor(L"UI", L"TextColor", m_ui.textColor, iniPath);
    WriteColor(L"UI", L"HighlightTextColor", m_ui.selTextColor, iniPath);
    WriteColor(L"UI", L"EditTextColor", m_ui.editTextColor, iniPath);
    WriteColor(L"UI", L"TitleColor", m_ui.titleColor, iniPath);

    SaveItemThresholds(iniPath);
}

void ConfigManager::SaveSnapshot() {
    if (!m_savePath.empty()) SaveItemThresholds(m_savePath);
}
void ConfigManager::LoadSnapshot() {
    if (!m_savePath.empty()) LoadItemThresholds(m_savePath);
}
void ConfigManager::ResetAll() {
    for (auto& item : m_items) { item.saleThreshold = m_defaultSale; item.buyThreshold = m_defaultBuy; }
}
void ConfigManager::SetItemSale(int index, int value) { if (index >= 0 && index < (int)m_items.size()) m_items[index].saleThreshold = value; }
void ConfigManager::SetItemBuy(int index, int value) { if (index >= 0 && index < (int)m_items.size()) m_items[index].buyThreshold = value; }
bool ConfigManager::CheckHotkey(const HotkeyConfig& hk) {
    bool ctrlOk = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? hk.ctrl : !hk.ctrl;
    bool shiftOk = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? hk.shift : !hk.shift;
    return (GetAsyncKeyState(hk.vkCode) & 0x8000) && ctrlOk && shiftOk;
}
