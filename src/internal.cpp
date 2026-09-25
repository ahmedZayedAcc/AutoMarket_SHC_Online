#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

#include <windows.h>
#include <intrin.h>

#include "MinHook.h"
#include "psutils.hpp"
#include "signature.hpp"
#include "internal.hpp"
#include "config.hpp" // لربط حالة زر الإيقاف المؤقت (Pause)


// ============================================================
// حالة داخلية عامة للملف (Pause + Virtual Inventory)
// لا يوجد هنا أي نظام logging - كل حاجة هنا منطق تشغيلي فعلي
// ============================================================

namespace
{
    // --------------------------------------------------------
    // حالة زر الإيقاف المؤقت (F4 أو أي هوت-كي محدد في config)
    // --------------------------------------------------------
    bool g_isTradePaused = false;

    void UpdatePauseState()
    {
        static bool wasPressed = false;
        bool isPressed = ConfigManager::Instance().CheckHotkey(ConfigManager::Instance().GetTogglePause());

        if (isPressed && !wasPressed)
        {
            g_isTradePaused = !g_isTradePaused;
        }
        wasPressed = isPressed;
    }


    // ========================================================
    // Virtual Inventory Tracker (Client-Side Prediction)
    //
    // بيعوّض التأخير الطبيعي بين إرسال أمر البيع/الشراء عبر
    // الشبكة ووصول التحديث الفعلي من اللعبة، عشان الرقم المعروض
    // في الواجهة يتحدث فورًا بدل ما ينتظر رد الشبكة.
    // ========================================================

    std::map<int, int> g_virtualDeltas;
    std::map<int, int> g_lastRealAmounts;
    std::map<int, DWORD> g_lastTradeTimes;

    int GetVirtualEffectiveAmount(
        int itemId,
        int realAmount
    )
    {
        if (g_lastRealAmounts.find(itemId) == g_lastRealAmounts.end())
        {
            g_lastRealAmounts[itemId] = realAmount;
        }

        DWORD now = GetTickCount();
        int diff = realAmount - g_lastRealAmounts[itemId];

        // الرقم الحقيقي اتغيّر معناه الشبكة استجابت أخيرًا لأوامرنا
        if (diff != 0)
        {
            if (g_virtualDeltas[itemId] > 0 && diff > 0)
            {
                g_virtualDeltas[itemId] -= diff;
                if (g_virtualDeltas[itemId] < 0) g_virtualDeltas[itemId] = 0;
            }
            else if (g_virtualDeltas[itemId] < 0 && diff < 0)
            {
                g_virtualDeltas[itemId] -= diff;
                if (g_virtualDeltas[itemId] > 0) g_virtualDeltas[itemId] = 0;
            }
            g_lastRealAmounts[itemId] = realAmount;
        }

        // أمان: لو الأمر ضاع في الشبكة، نصفّر الرقم الوهمي بعد ثانية
        if (
            g_virtualDeltas[itemId] != 0
            &&
            (now - g_lastTradeTimes[itemId] > 1000)
        )
        {
            g_virtualDeltas[itemId] = 0;
        }

        return realAmount + g_virtualDeltas[itemId];
    }


    // ========================================================
    // Market Action Sender — المسار الحقيقي المؤكد (0x466A10)
    //
    // ده هو الدالة اللي بتاخد نفس مسار الشبكة اللي اللاعب
    // البشري بيمر بيه فعليًا (466A10 -> Command 0x26 -> 4884C0
    // -> 487010 -> DirectPlay -> Host). AutoMarket بيكتب الـItemID
    // المطلوب في مكان "العنصر المختار حاليًا" بتاع اللاعب، وبعدين
    // بينده الدالة دي بـmode=0(Buy) أو mode=1(Sell) بس.
    //
    // arg1 = mode (0=Buy, 1=Sell)
    // ========================================================

    typedef void (__cdecl* MarketActionFn)(int mode);

    MarketActionFn g_marketAction = nullptr;
    bool g_marketActionInitialized = false;


    // ========================================================
    // قراءة/كتابة عدد صحيح من ذاكرة اللعبة مباشرة
    // ========================================================

    int ReadInt(uintptr_t address)
    {
        return *reinterpret_cast<int*>(address);
    }

    void WriteInt(uintptr_t address, int value)
    {
        *reinterpret_cast<int*>(address) = value;
    }


    // ========================================================
    // تنفيذ عملية التجارة الفعلية (Buy أو Sell)
    //
    // 1) يتأكد إن التجارة مش متوقفة مؤقتًا (Pause)
    // 2) Cooldown 20ms لمنع كراش اللعبة عند الإرسال السريع جدًا
    //    (النقطة دي متعمّد إنها متتغيرش)
    // 3) يكتب الـItemID المطلوب في خانة "العنصر المختار حاليًا"
    //    الخاصة باللاعب الحالي (115AE18 + playerId*0x39f4)
    // 4) ينده 466A10 بالـmode المطلوب — ده اللي بيبعت Command 0x26
    //    فعليًا عبر نفس مسار الشبكة الحقيقي
    // 5) يحدّث الـVirtual Tracker فورًا عشان الواجهة تستجيب بسرعة
    // ========================================================

    void ExecuteHumanMarketTrade(
        int buySell,
        int itemId,
        int quantity
    )
    {
        if (g_isTradePaused)
        {
            return;
        }

        DWORD currentTime = GetTickCount();
        if (currentTime - g_lastTradeTimes[itemId] < 20)
        {
            return;
        }

        const uintptr_t baseAddress = psutils::getBaseAddress();
        if (baseAddress == 0)
        {
            return;
        }

        if (!g_marketActionInitialized || !g_marketAction)
        {
            return;
        }

        const int playerId = ReadInt(baseAddress + 0x16260A4);

        const uintptr_t currentItemAddr =
            baseAddress
            + 0xD5AE18
            + (playerId * 0x39f4);

        WriteInt(currentItemAddr, itemId);

        // === الإرسال الفعلي عبر المسار الحقيقي (466A10) ===
        g_marketAction(buySell);

        if (buySell == 0)
        {
            g_virtualDeltas[itemId] += quantity; // شراء
        }
        else
        {
            g_virtualDeltas[itemId] -= quantity; // بيع
        }

        g_lastTradeTimes[itemId] = GetTickCount();
    }
}


// ============================================================
// HasMarket — هل يوجد سوق مبني للاعب ده؟
// ============================================================

bool GameInterface::HasMarket(int playerId)
{
    if (playerId < 0 || playerId > 8)
    {
        return false;
    }

    if (marketIdAddress == 0)
    {
        return false;
    }

    return *reinterpret_cast<int*>(marketIdAddress + (0x39f4 * playerId)) != 0;
}


// ============================================================
// GetPlayerId — قراءة رقم اللاعب الحالي مباشرة من متغيّر اللعبة
// (0x1A260A4) بدل الاعتماد على قيمة ثابتة
// ============================================================

int GameInterface::GetPlayerId()
{
    const uintptr_t baseAddress = psutils::getBaseAddress();

    if (baseAddress == 0)
    {
        return 0;
    }

    return *reinterpret_cast<int*>(baseAddress + 0x16260A4);
}


// ============================================================
// تهيئة كل العناوين المطلوبة عن طريق الـsignatures
// ============================================================

void GameInterface::initializeAddresses()
{
    const std::string tradeFuncAddressSignature =
        "8B ?? ?? ?? 85 ?? ?? ?? ?? 75 ?? 8B ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??";

    const std::string marketIdAddressPointerSignature =
        "?? ?? ?? ?? E9 ?? ?? ?? ?? 0F BF ?? ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? F7 ?? B9 ?? ?? ?? ?? 89 ?? ??";

    const std::string productBaseAddressPointerSignature =
        "?? ?? ?? ?? 83 ?? ?? 83 ?? ?? 7C ?? B8 ?? ?? ?? ?? 39 ?? ?? 89 ?? ?? ?? 0F 8E ?? ?? ?? ?? 8D ??";

    const std::string processName = psutils::getProcessName();
    const uintptr_t baseAddress = psutils::getBaseAddress();

    const uintptr_t tradeFuncAddress =
        baseAddress + signature::getAddressBySignatureInFile(processName, tradeFuncAddressSignature);

    const uintptr_t marketIdAddressPointer =
        baseAddress + signature::getAddressBySignatureInFile(processName, marketIdAddressPointerSignature);

    const uintptr_t productBaseAddressPointer =
        baseAddress + signature::getAddressBySignatureInFile(processName, productBaseAddressPointerSignature);

    if (
        !psutils::isValidAddress(tradeFuncAddress) ||
        !psutils::isValidAddress(marketIdAddressPointer) ||
        !psutils::isValidAddress(productBaseAddressPointer)
    )
    {
        // ده مش "logging" — ده تنبيه فعلي للمستخدم إن المود
        // مش هيشتغل خالص، فسبته زي ما هو
        MessageBoxW(NULL, L"Error: One or more signatures not found or invalid addresses", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // محتفظ بيه للتوافق مع أي كود تاني بيستخدم tradeFunction مباشرة،
    // حتى لو AutoMarket نفسه بقى بيستخدم 466A10 بدلاً منها
    tradeFunction = reinterpret_cast<void (*)(int, bool, int)>(tradeFuncAddress);

    marketIdAddress = *reinterpret_cast<int*>(marketIdAddressPointer);
    productBaseAddress = *reinterpret_cast<int*>(productBaseAddressPointer);

    // --------------------------------------------------------
    // تهيئة الـsender الحقيقي (466A10)
    // --------------------------------------------------------
    const uintptr_t marketActionAddress = baseAddress + 0x66A10;

    if (psutils::isValidAddress(marketActionAddress))
    {
        g_marketAction = reinterpret_cast<MarketActionFn>(marketActionAddress);
        g_marketActionInitialized = true;
    }
    else
    {
        g_marketAction = nullptr;
        g_marketActionInitialized = false;
    }
}


// ============================================================
// InstallHumanMarketTrace / RemoveHumanMarketTrace
//
// كانت بتركّب MinHook hooks للتتبع/الـlogging فقط (مالهاش أي
// تأثير على منطق التجارة نفسه). اتشالت بالكامل بناءً على طلب
// إلغاء الـlogging، لكن الدالتين اتسابتا موجودتين كـno-op عشان
// أي استدعاء ليهم من مكان تاني في المشروع (زي وقت تحميل الـDLL)
// يفضل يشتغل من غير أي تعديل أو كسر.
// ============================================================

bool GameInterface::InstallHumanMarketTrace()
{
    return true;
}

void GameInterface::RemoveHumanMarketTrace()
{
}


// ============================================================
// Sell Product
// ============================================================

void GameInterface::sellProduct(const std::string& productName)
{
    int playerId = GetPlayerId();

    if (playerId <= 0)
    {
        playerId = 1;
    }

    if (!HasMarket(playerId))
    {
        return;
    }

    const auto it = productIds.find(productName);
    if (it == productIds.end())
    {
        return;
    }

    // الأسلحة بتتباع بالقطعة، الموارد بتتباع بالـ5
    bool isWeapon = (
        productName == "bows" || productName == "crossbows" ||
        productName == "leather armor" || productName == "maces" ||
        productName == "metal armor" || productName == "pikes" ||
        productName == "spears" || productName == "swords"
    );
    int quantity = isWeapon ? 1 : 5;

    ExecuteHumanMarketTrade(1, it->second, quantity);
}


// ============================================================
// Buy Product
// ============================================================

void GameInterface::buyProduct(const std::string& productName)
{
    int playerId = GetPlayerId();

    if (playerId <= 0)
    {
        playerId = 1;
    }

    if (!HasMarket(playerId))
    {
        return;
    }

    const auto it = productIds.find(productName);
    if (it == productIds.end())
    {
        return;
    }

    bool isWeapon = (
        productName == "bows" || productName == "crossbows" ||
        productName == "leather armor" || productName == "maces" ||
        productName == "metal armor" || productName == "pikes" ||
        productName == "spears" || productName == "swords"
    );
    int quantity = isWeapon ? 1 : 5;

    ExecuteHumanMarketTrade(0, it->second, quantity);
}


// ============================================================
// Get Number Products — قراءة كمية المخزون المعروضة في الواجهة
// (الرقم الحقيقي من اللعبة + تعديل الـVirtual Tracker)
// ============================================================

int GameInterface::getNumberProducts(const std::string& productName)
{
    UpdatePauseState(); // تحديث حالة زرار الإيقاف قبل أي قراءة

    int playerId = GetPlayerId();
    if (playerId <= 0)
    {
        playerId = 1;
    }

    int inventoryIndex = playerId - 1;

    if (productBaseAddress == 0)
    {
        return 0;
    }

    const auto it = productIds.find(productName);
    if (it == productIds.end())
    {
        return 0;
    }

    int realAmount = *reinterpret_cast<int*>(
        productBaseAddress
        + (0x39f4 * inventoryIndex)
        + ((0xe7d + it->second) * 0x4)
    );

    return GetVirtualEffectiveAmount(it->second, realAmount);
}


// ============================================================
// Available Products
// ============================================================

std::vector<std::string> GameInterface::getAvailableProducts() const
{
    std::vector<std::string> availableProducts;
    for (const auto& pair : productIds)
    {
        availableProducts.push_back(pair.first);
    }
    return availableProducts;
}


// ============================================================
// Constructor
// ============================================================

GameInterface::GameInterface()
{
    initializeAddresses();
}


// ============================================================
// Is In Game
// ============================================================

bool GameInterface::isInGame()
{
    return HasMarket(GetPlayerId());
}
