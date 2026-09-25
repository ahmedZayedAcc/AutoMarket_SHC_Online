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
#include "config.hpp" // تم إضافته لربط حالة الزر


// ============================================================
// Global logging / tracing state & Hybrid System
// ============================================================

namespace
{

    std::ofstream g_log;

    DWORD g_lastPlayerScan = 0;
    
    // متغير حالة إيقاف التجارة
    bool g_isTradePaused = false;

    // تعريف مبدئي لدالة الطباعة حتى تراها UpdatePauseState
    void WriteLog(const std::string& text);

    // دالة فحص وتحديث حالة زر الإيقاف المؤقت
    void UpdatePauseState()
    {
        static bool wasPressed = false;
        // تم استدعاء GetTogglePause() بدلاً من m_togglePause مباشرة
        bool isPressed = ConfigManager::Instance().CheckHotkey(ConfigManager::Instance().GetTogglePause());

        if (isPressed && !wasPressed)
        {
            g_isTradePaused = !g_isTradePaused;
            if (g_isTradePaused)
            {
                WriteLog("[PAUSE] AutoMarket Trading is PAUSED by user (F4)");
            }
            else
            {
                WriteLog("[PAUSE] AutoMarket Trading is RESUMED by user (F4)");
            }
        }
        wasPressed = isPressed;
    }

    // ========================================================
    // Virtual Inventory Tracker (Client-Side Prediction)
    // ========================================================
    std::map<int, int> g_virtualDeltas;
    std::map<int, int> g_lastRealAmounts;
    std::map<int, DWORD> g_lastTradeTimes;

    int GetVirtualEffectiveAmount(
        int itemId,
        int realAmount
    )
    {
        // تهيئة القيمة لأول مرة
        if (
            g_lastRealAmounts.find(itemId) == g_lastRealAmounts.end()
        )
        {
            g_lastRealAmounts[itemId] = realAmount;
        }

        DWORD now = GetTickCount();
        int diff = realAmount - g_lastRealAmounts[itemId];

        // إذا تغير الرقم الحقيقي (بمعنى أن الشبكة استجابت أخيراً لأوامرنا)
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

        // نظام أمان: إذا ضاع الأمر في الشبكة، نصفر الرقم الوهمي بعد ثانية
        if (
            g_virtualDeltas[itemId] != 0
            &&
            (now - g_lastTradeTimes[itemId] > 1000)
        )
        {
            g_virtualDeltas[itemId] = 0;
        }

        // المخزون الفعلي = الحقيقي من اللعبة + الوهمي المعلق في الشبكة
        return realAmount + g_virtualDeltas[itemId];
    }


    // ========================================================
    // Original Trade Function
    //
    // Stronghold Crusader 1.3
    //
    // 0x465830
    //
    // Arguments:
    //
    //     arg1 = player slot
    //     arg2 = Buy/Sell
    //            0 = Buy
    //            1 = Sell
    //     arg3 = item ID
    //
    // This is kept for TRACE / observation.
    //
    // AutoMarket will no longer call 465830 directly.
    // ========================================================

    typedef void (__cdecl* TradeFunctionFn)(
        int playerSlot,
        bool buySell,
        int itemId
    );


    TradeFunctionFn
        g_originalTradeFunction = nullptr;


    bool g_tradeTraceInstalled = false;


    // ========================================================
    // Market Action Sender
    //
    // Stronghold Crusader 1.3
    //
    // Absolute VA:
    //
    //     0x466A10
    //
    // RVA:
    //
    //     0x66A10
    //
    // Static path:
    //
    //     AutoMarket
    //        |
    //        v
    //     Write ItemID to 115AE18 (Current Market Selection)
    //        |
    //        v
    //     466A10(mode)
    //        |
    //        v
    //     Original game checks (Gold, Stock, etc.)
    //        |
    //        v
    //     Command 0x26
    //        |
    //        v
    //     DirectPlay -> Host
    //
    // The function receives the Mode as its argument:
    //     0 = BUY
    //     1 = SELL
    //
    // ========================================================

    typedef void (__cdecl* MarketActionFn)(
        int mode
    );


    MarketActionFn
        g_marketAction = nullptr;


    bool g_marketActionInitialized = false;


    // ========================================================
    // Original 4884C0
    //
    // 4884C0 is the network command preparation/sender layer.
    //
    // Observed call pattern:
    //
    //     push command
    //     mov ecx,0191C378
    //     call 004884C0
    //
    // Hook representation:
    //
    //     __thiscall
    //
    // ECX = network manager object
    // arg1 = command
    // ========================================================

    typedef void (__thiscall* NetworkSendFunctionFn)(
        void* thisPtr,
        int command
    );


    NetworkSendFunctionFn
        g_originalNetworkSendFunction = nullptr;


    bool g_networkTraceInstalled = false;


    // ========================================================
    // Logging
    // ========================================================

    void WriteLog(
        const std::string& text
    )
    {
        if (!g_log.is_open())
        {
            g_log.open(
                "automarket_debug.log",
                std::ios::app
            );
        }

        if (g_log.is_open())
        {
            g_log
                << text
                << std::endl;

            g_log.flush();
        }

        OutputDebugStringA(
            (
                text
                + "\n"
            ).c_str()
        );
    }


    // ========================================================
    // Read integer from game memory
    // ========================================================

    int ReadInt(
        uintptr_t address
    )
    {
        return *reinterpret_cast<int*>(
            address
        );
    }


    // ========================================================
    // Write integer to game memory
    // ========================================================

    void WriteInt(
        uintptr_t address,
        int value
    )
    {
        *reinterpret_cast<int*>(
            address
        ) = value;
    }


    // ========================================================
    // Network Send Hook
    //
    // IMPORTANT:
    //
    // This version logs EVERY command.
    //
    // Now logs detailed state for 0x26 (Market).
    //
    // This lets us verify that AutoMarket -> 466A10
    // actually reaches 4884C0.
    // ========================================================

    void __fastcall HookNetworkSend(
        void* thisPtr,
        void* /* edx */,
        int command
    )
    {
        const uintptr_t baseAddress =
            psutils::getBaseAddress();


        const uintptr_t caller =
            reinterpret_cast<uintptr_t>(
                __builtin_return_address(0)
            );


        std::ostringstream out;

        out
            << "\n"
            << "============================================================\n"
            << "[NETWORK_4884C0]\n"
            << "============================================================\n"

            << "Command=0x"
            << std::hex
            << command
            << std::dec
            << " ("
            << command
            << ")\n"

            << "Caller=0x"
            << std::hex
            << caller
            << std::dec
            << "\n"

            << "CallerRVA=0x"
            << std::hex
            << (
                caller >= baseAddress
                ? caller - baseAddress
                : caller
            )
            << std::dec
            << "\n"

            << "ECX=0x"
            << std::hex
            << reinterpret_cast<uintptr_t>(
                thisPtr
            )
            << std::dec
            << "\n";


        // ====================================================
        // Additional market state for Command 0x26
        // ====================================================

        if (command == 0x26)
        {
            const uintptr_t addr1125554 =
                baseAddress + 0xD25554;

            const uintptr_t addr1125558 =
                baseAddress + 0xD25558;

            const uintptr_t addr1125560 =
                baseAddress + 0xD25560;

            const uintptr_t addr1996AD0 =
                baseAddress + 0x1596AD0;

            const uintptr_t addr1996AD4 =
                baseAddress + 0x1596AD4;

            const uintptr_t addr1996AD8 =
                baseAddress + 0x1596AD8;

            const uintptr_t addr1A260A0 =
                baseAddress + 0x16260A0;

            const uintptr_t addr1A260A4 =
                baseAddress + 0x16260A4;


            out
                << "1125554="
                << ReadInt(addr1125554)
                << "\n"

                << "1125558="
                << ReadInt(addr1125558)
                << "\n"

                << "1125560="
                << ReadInt(addr1125560)
                << "\n"

                << "1996AD0="
                << ReadInt(addr1996AD0)
                << "\n"

                << "1996AD4="
                << ReadInt(addr1996AD4)
                << "\n"

                << "1996AD8="
                << ReadInt(addr1996AD8)
                << "\n"

                << "1A260A0="
                << ReadInt(addr1A260A0)
                << "\n"

                << "1A260A4="
                << ReadInt(addr1A260A4)
                << "\n";
        }


        out
            << "============================================================";


        WriteLog(
            out.str()
        );


        // ====================================================
        // Original 4884C0
        // ====================================================

        if (
            g_originalNetworkSendFunction
        )
        {
            g_originalNetworkSendFunction(
                thisPtr,
                command
            );
        }
    }


    // ========================================================
    // Trade Function Hook
    //
    // 465830
    //
    // This is observation only.
    //
    // It is intentionally NOT used as the AutoMarket sender.
    // ========================================================

    void __cdecl HookTradeFunction(
        int playerSlot,
        bool buySell,
        int itemId
    )
    {
        const uintptr_t baseAddress =
            psutils::getBaseAddress();


        const uintptr_t addr1125554 =
            baseAddress + 0xD25554;

        const uintptr_t addr1125558 =
            baseAddress + 0xD25558;

        const uintptr_t addr1125560 =
            baseAddress + 0xD25560;

        const uintptr_t addr1996AD0 =
            baseAddress + 0x1596AD0;

        const uintptr_t addr1996AD4 =
            baseAddress + 0x1596AD4;

        const uintptr_t addr1996AD8 =
            baseAddress + 0x1596AD8;

        const uintptr_t addr1A260A0 =
            baseAddress + 0x16260A0;

        const uintptr_t addr1A260A4 =
            baseAddress + 0x16260A4;


        const uintptr_t caller =
            reinterpret_cast<uintptr_t>(
                __builtin_return_address(0)
            );


        const char* tradeType =
            buySell
            ? "SELL"
            : "BUY";


        std::ostringstream out;

        out
            << "\n"
            << "============================================================\n"
            << "[TRADE_465830]\n"
            << "============================================================\n"

            << "Caller=0x"
            << std::hex
            << caller
            << std::dec
            << "\n"

            << "CallerRVA=0x"
            << std::hex
            << (
                caller >= baseAddress
                ? caller - baseAddress
                : caller
            )
            << std::dec
            << "\n"

            << "PlayerSlot="
            << playerSlot
            << "\n"

            << "BuySell="
            << static_cast<int>(
                buySell
            )
            << "\n"

            << "TradeType="
            << tradeType
            << "\n"

            << "ItemId="
            << itemId
            << "\n"

            << "1125554="
            << ReadInt(addr1125554)
            << "\n"

            << "1125558="
            << ReadInt(addr1125558)
            << "\n"

            << "1125560="
            << ReadInt(addr1125560)
            << "\n"

            << "1996AD0="
            << ReadInt(addr1996AD0)
            << "\n"

            << "1996AD4="
            << ReadInt(addr1996AD4)
            << "\n"

            << "1996AD8="
            << ReadInt(addr1996AD8)
            << "\n"

            << "1A260A0="
            << ReadInt(addr1A260A0)
            << "\n"

            << "1A260A4="
            << ReadInt(addr1A260A4)
            << "\n"

            << "============================================================";


        WriteLog(
            out.str()
        );


        // ====================================================
        // Original 465830
        // ====================================================

        if (
            g_originalTradeFunction
        )
        {
            g_originalTradeFunction(
                playerSlot,
                buySell,
                itemId
            );
        }
    }


    // ========================================================
    // Player scan
    // ========================================================

    void ScanPlayersWood(
        uintptr_t productBaseAddress,
        uintptr_t marketIdAddress
    )
    {
        DWORD now =
            GetTickCount();


        if (
            now - g_lastPlayerScan
            < 1000
        )
        {
            return;
        }


        g_lastPlayerScan =
            now;


        std::ostringstream out;

        out
            << "[PLAYER_SCAN]";


        for (
            int playerId = 1;
            playerId <= 8;
            ++playerId
        )
        {
            uintptr_t marketAddress =
                marketIdAddress
                +
                (
                    0x39f4
                    *
                    playerId
                );


            int inventoryIndex =
                playerId - 1;


            uintptr_t woodAddress =
                productBaseAddress
                +
                (
                    0x39f4
                    *
                    inventoryIndex
                )
                +
                (
                    (0xe7d + 2)
                    *
                    0x4
                );


            int marketValue =
                *reinterpret_cast<int*>(
                    marketAddress
                );


            int woodAmount =
                *reinterpret_cast<int*>(
                    woodAddress
                );


            out
                << " P"
                << playerId
                << "(Market="
                << marketValue
                << ",Wood="
                << woodAmount
                << ")";
        }


        WriteLog(
            out.str()
        );
    }

}


// ============================================================
// HasMarket
// ============================================================

bool GameInterface::HasMarket(
    int playerId
)
{
    if (
        playerId < 0
        ||
        playerId > 8
    )
    {
        return false;
    }


    if (
        marketIdAddress == 0
    )
    {
        return false;
    }


    return
        *reinterpret_cast<int*>(
            marketIdAddress
            +
            (
                0x39f4
                *
                playerId
            )
        )
        != 0;
}


// ============================================================
// GetPlayerId
// ============================================================

int GameInterface::GetPlayerId()
{
    const uintptr_t baseAddress =
        psutils::getBaseAddress();


    if (
        baseAddress == 0
    )
    {
        return 0;
    }


    int playerId =
        *reinterpret_cast<int*>(
            baseAddress
            +
            0x16260A4
        );


    static int lastPlayerId =
        -999999;


    if (
        playerId
        !=
        lastPlayerId
    )
    {
        std::ostringstream out;

        out
            << "[PLAYER_ID_CHANGED]"
            << " Address=0x"
            << std::hex
            << (
                baseAddress
                +
                0x16260A4
            )
            << std::dec
            << " PlayerID="
            << playerId;


        WriteLog(
            out.str()
        );


        lastPlayerId =
            playerId;
    }


    return playerId;
}


// ============================================================
// Initialize addresses
// ============================================================

void GameInterface::initializeAddresses()
{
    const std::string tradeFuncAddressSignature =
        "8B ?? ?? ?? 85 ?? ?? ?? ?? 75 ?? 8B ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??";


    const std::string marketIdAddressPointerSignature =
        "?? ?? ?? ?? E9 ?? ?? ?? ?? 0F BF ?? ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? F7 ?? B9 ?? ?? ?? ?? 89 ?? ??";


    const std::string productBaseAddressPointerSignature =
        "?? ?? ?? ?? 83 ?? ?? 83 ?? ?? 7C ?? B8 ?? ?? ?? ?? 39 ?? ?? 89 ?? ?? ?? 0F 8E ?? ?? ?? ?? 8D ??";


    const std::string processName =
        psutils::getProcessName();


    const uintptr_t baseAddress =
        psutils::getBaseAddress();


    const uintptr_t tradeFuncAddress =
        baseAddress
        +
        signature::getAddressBySignatureInFile(
            processName,
            tradeFuncAddressSignature
        );


    const uintptr_t marketIdAddressPointer =
        baseAddress
        +
        signature::getAddressBySignatureInFile(
            processName,
            marketIdAddressPointerSignature
        );


    const uintptr_t productBaseAddressPointer =
        baseAddress
        +
        signature::getAddressBySignatureInFile(
            processName,
            productBaseAddressPointerSignature
        );


    if (
        !psutils::isValidAddress(
            tradeFuncAddress
        )
        ||
        !psutils::isValidAddress(
            marketIdAddressPointer
        )
        ||
        !psutils::isValidAddress(
            productBaseAddressPointer
        )
    )
    {
        MessageBoxW(
            NULL,
            L"Error: One or more signatures not found or invalid addresses",
            L"Error",
            MB_OK | MB_ICONERROR
        );


        return;
    }


    // ========================================================
    // Original Trade Function
    //
    // Kept for tracing / observation.
    // ========================================================

    tradeFunction =
        reinterpret_cast<
            void (*)(int, bool, int)
        >(
            tradeFuncAddress
        );


    marketIdAddress =
        *reinterpret_cast<int*>(
            marketIdAddressPointer
        );


    productBaseAddress =
        *reinterpret_cast<int*>(
            productBaseAddressPointer
        );


    // ========================================================
    // Initialize Market Action
    //
    // 0x466A10
    // RVA 0x66A10
    // ========================================================

    const uintptr_t marketActionAddress =
        baseAddress
        +
        0x66A10;


    if (
        psutils::isValidAddress(
            marketActionAddress
        )
    )
    {
        g_marketAction =
            reinterpret_cast<
                MarketActionFn
            >(
                marketActionAddress
            );

        g_marketActionInitialized =
            true;


        std::ostringstream senderLog;

        senderLog
            << "[MARKET_ACTION]"
            << " Address=0x"
            << std::hex
            << marketActionAddress
            << std::dec;


        WriteLog(
            senderLog.str()
        );
    }
    else
    {
        g_marketAction =
            nullptr;

        g_marketActionInitialized =
            false;


        WriteLog(
            "[MARKET_ACTION] Invalid 0x466A10 address"
        );
    }


    // ========================================================
    // Open log
    // ========================================================

    g_log.open(
        "automarket_debug.log",
        std::ios::out
        |
        std::ios::trunc
    );


    std::ostringstream out;

    out
        << "[INITIALIZE_ADDRESSES]\n"

        << "ProcessName="
        << processName
        << "\n"

        << "BaseAddress=0x"
        << std::hex
        << baseAddress
        << "\n"

        << "TradeFuncAddress=0x"
        << tradeFuncAddress
        << "\n"

        << "MarketAction=0x"
        << (
            baseAddress
            +
            0x66A10
        )
        << "\n"

        << "MarketIdAddressPointer=0x"
        << marketIdAddressPointer
        << "\n"

        << "MarketIdAddress=0x"
        << marketIdAddress
        << "\n"

        << "ProductBaseAddressPointer=0x"
        << productBaseAddressPointer
        << "\n"

        << "ProductBaseAddress=0x"
        << productBaseAddress
        << std::dec;


    WriteLog(
        out.str()
    );
}


// ============================================================
// Install Trade + Network Trace
// ============================================================

bool GameInterface::InstallHumanMarketTrace()
{
    if (
        g_tradeTraceInstalled
        &&
        g_networkTraceInstalled
    )
    {
        return true;
    }


    const uintptr_t baseAddress =
        psutils::getBaseAddress();


    if (
        baseAddress == 0
    )
    {
        WriteLog(
            "[TRACE] Invalid base address"
        );


        return false;
    }


    const uintptr_t tradeAddress =
        baseAddress
        +
        0x65830;


    const uintptr_t networkAddress =
        baseAddress
        +
        0x884C0;


    if (
        !psutils::isValidAddress(
            tradeAddress
        )
    )
    {
        WriteLog(
            "[TRADE_TRACE] Invalid 0x465830 address"
        );


        return false;
    }


    if (
        !psutils::isValidAddress(
            networkAddress
        )
    )
    {
        WriteLog(
            "[NETWORK_TRACE] Invalid 0x4884C0 address"
        );


        return false;
    }


    // ========================================================
    // Trade hook
    // ========================================================

    if (
        !g_tradeTraceInstalled
    )
    {
        MH_STATUS status =
            MH_CreateHook(
                reinterpret_cast<LPVOID>(
                    tradeAddress
                ),

                reinterpret_cast<LPVOID>(
                    &HookTradeFunction
                ),

                reinterpret_cast<LPVOID*>(
                    &g_originalTradeFunction
                )
            );


        if (
            status
            !=
            MH_OK
        )
        {
            std::ostringstream out;

            out
                << "[TRADE_TRACE]"
                << " MH_CreateHook failed: "
                << MH_StatusToString(
                    status
                );


            WriteLog(
                out.str()
            );


            return false;
        }


        status =
            MH_EnableHook(
                reinterpret_cast<LPVOID>(
                    tradeAddress
                )
            );


        if (
            status
            !=
            MH_OK
        )
        {
            std::ostringstream out;

            out
                << "[TRADE_TRACE]"
                << " MH_EnableHook failed: "
                << MH_StatusToString(
                    status
                );


            WriteLog(
                out.str()
            );


            MH_RemoveHook(
                reinterpret_cast<LPVOID>(
                    tradeAddress
                )
            );


            g_originalTradeFunction =
                nullptr;


            return false;
        }


        g_tradeTraceInstalled =
            true;


        std::ostringstream out;

        out
            << "[TRADE_TRACE_INSTALLED]"
            << " Address=0x"
            << std::hex
            << tradeAddress
            << std::dec;


        WriteLog(
            out.str()
        );
    }


    // ========================================================
    // Network hook
    // ========================================================

    if (
        !g_networkTraceInstalled
    )
    {
        MH_STATUS status =
            MH_CreateHook(
                reinterpret_cast<LPVOID>(
                    networkAddress
                ),

                reinterpret_cast<LPVOID>(
                    &HookNetworkSend
                ),

                reinterpret_cast<LPVOID*>(
                    &g_originalNetworkSendFunction
                )
            );


        if (
            status
            !=
            MH_OK
        )
        {
            std::ostringstream out;

            out
                << "[NETWORK_TRACE]"
                << " MH_CreateHook failed: "
                << MH_StatusToString(
                    status
                );


            WriteLog(
                out.str()
            );


            if (
                g_tradeTraceInstalled
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        tradeAddress
                    )
                );

                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        tradeAddress
                    )
                );


                g_originalTradeFunction =
                    nullptr;

                g_tradeTraceInstalled =
                    false;
            }


            return false;
        }


        status =
            MH_EnableHook(
                reinterpret_cast<LPVOID>(
                    networkAddress
                )
            );


        if (
            status
            !=
            MH_OK
        )
        {
            std::ostringstream out;

            out
                << "[NETWORK_TRACE]"
                << " MH_EnableHook failed: "
                << MH_StatusToString(
                    status
                );


            WriteLog(
                out.str()
            );


            MH_RemoveHook(
                reinterpret_cast<LPVOID>(
                    networkAddress
                )
            );


            g_originalNetworkSendFunction =
                nullptr;


            if (
                g_tradeTraceInstalled
            )
            {
                MH_DisableHook(
                    reinterpret_cast<LPVOID>(
                        tradeAddress
                    )
                );

                MH_RemoveHook(
                    reinterpret_cast<LPVOID>(
                        tradeAddress
                    )
                );


                g_originalTradeFunction =
                    nullptr;

                g_tradeTraceInstalled =
                    false;
            }


            return false;
        }


        g_networkTraceInstalled =
            true;


        std::ostringstream out;

        out
            << "[NETWORK_TRACE_INSTALLED]"
            << " Address=0x"
            << std::hex
            << networkAddress
            << std::dec;


        WriteLog(
            out.str()
        );
    }


    return true;
}


// ============================================================
// Remove Trade + Network Trace
// ============================================================

void GameInterface::RemoveHumanMarketTrace()
{
    const uintptr_t baseAddress =
        psutils::getBaseAddress();


    if (
        g_networkTraceInstalled
    )
    {
        const uintptr_t networkAddress =
            baseAddress
            +
            0x884C0;


        MH_DisableHook(
            reinterpret_cast<LPVOID>(
                networkAddress
            )
        );


        MH_RemoveHook(
            reinterpret_cast<LPVOID>(
                networkAddress
            )
        );


        g_originalNetworkSendFunction =
            nullptr;


        g_networkTraceInstalled =
            false;


        WriteLog(
            "[NETWORK_TRACE_REMOVED]"
        );
    }


    if (
        g_tradeTraceInstalled
    )
    {
        const uintptr_t tradeAddress =
            baseAddress
            +
            0x65830;


        MH_DisableHook(
            reinterpret_cast<LPVOID>(
                tradeAddress
            )
        );


        MH_RemoveHook(
            reinterpret_cast<LPVOID>(
                tradeAddress
            )
        );


        g_originalTradeFunction =
            nullptr;


        g_tradeTraceInstalled =
            false;


        WriteLog(
            "[TRADE_TRACE_REMOVED]"
        );
    }
}


// ============================================================
// Market Action Trade (Modified with Hybrid System)
// ============================================================

void ExecuteHumanMarketTrade(
    int buySell,
    int itemId,
    int quantity
)
{
    // منع التنفيذ إذا كان وضع الإيقاف المؤقت (Pause) مفعلاً
    if (g_isTradePaused)
    {
        return;
    }

    // ========================================================
    // كول داون سريع جداً جداً لمنع كراش اللعبة (20 مللي ثانية)
    // ========================================================
    DWORD currentTime = GetTickCount();
    if (
        currentTime - g_lastTradeTimes[itemId] < 20
    )
    {
        return;
    }


    const uintptr_t baseAddress =
        psutils::getBaseAddress();


    if (
        baseAddress == 0
    )
    {
        WriteLog(
            "[HUMAN_MARKET_TRADE] Invalid base address"
        );


        return;
    }


    if (
        !g_marketActionInitialized
        ||
        !g_marketAction
    )
    {
        WriteLog(
            "[HUMAN_MARKET_TRADE] Sender 0x466A10 not initialized"
        );


        return;
    }


    // ========================================================
    // Get Player ID
    // ========================================================

    const int playerId =
        ReadInt(
            baseAddress
            +
            0x16260A4
        );


    // ========================================================
    // 115AE18 - Current Market Item ID for Player
    // ========================================================

    const uintptr_t currentItemAddr =
        baseAddress
        +
        0xD5AE18
        +
        (
            playerId
            *
            0x39f4
        );


    // ========================================================
    // Log BEFORE sending
    // ========================================================

    {
        std::ostringstream out;

        out
            << "[HUMAN_MARKET_TRADE]"
            << " Sender=0x"
            << std::hex
            << (
                baseAddress
                +
                0x66A10
            )
            << std::dec

            << " BuySell="
            << buySell

            << " ItemId="
            << itemId
            
            << " Quantity="
            << quantity

            << " PlayerId="
            << playerId;


        WriteLog(
            out.str()
        );
    }


    // ========================================================
    // Write Item ID to current market selection
    // ========================================================

    WriteInt(
        currentItemAddr,
        itemId
    );


    // ========================================================
    // ORIGINAL MARKET ACTION
    // ========================================================

    g_marketAction(
        buySell
    );


    // ========================================================
    // Update Virtual Tracker Instantly
    // ========================================================

    if (buySell == 0)
    {
        // شراء
        g_virtualDeltas[itemId] += quantity;
    }
    else
    {
        // بيع
        g_virtualDeltas[itemId] -= quantity;
    }

    g_lastTradeTimes[itemId] = GetTickCount();


    // ========================================================
    // Log AFTER sender returns
    // ========================================================

    {
        std::ostringstream out;

        out
            << "[HUMAN_MARKET_TRADE_DONE]"
            << " BuySell="
            << buySell

            << " ItemId="
            << itemId;


        WriteLog(
            out.str()
        );
    }
}


// ============================================================
// Sell Product
// ============================================================

void GameInterface::sellProduct(
    const std::string& productName
)
{
    int playerId =
        GetPlayerId();


    if (
        playerId <= 0
    )
    {
        playerId = 1;
    }


    if (
        !HasMarket(
            playerId
        )
    )
    {
        return;
    }


    const auto it =
        productIds.find(
            productName
        );


    if (
        it == productIds.end()
    )
    {
        return;
    }

    
    // تحديد ما إذا كان العنصر سلاحاً (يباع بالقطعة) أو مورداً (يباع بالـ 5)
    bool isWeapon = (
        productName == "bows" || productName == "crossbows" || 
        productName == "leather armor" || productName == "maces" || 
        productName == "metal armor" || productName == "pikes" || 
        productName == "spears" || productName == "swords"
    );
    int quantity = isWeapon ? 1 : 5;


    ExecuteHumanMarketTrade(
        1,
        it->second,
        quantity
    );
}


// ============================================================
// Buy Product
// ============================================================

void GameInterface::buyProduct(
    const std::string& productName
)
{
    int playerId =
        GetPlayerId();


    if (
        playerId <= 0
    )
    {
        playerId = 1;
    }


    if (
        !HasMarket(
            playerId
        )
    )
    {
        return;
    }


    const auto it =
        productIds.find(
            productName
        );


    if (
        it == productIds.end()
    )
    {
        return;
    }


    // تحديد ما إذا كان العنصر سلاحاً (يباع بالقطعة) أو مورداً (يباع بالـ 5)
    bool isWeapon = (
        productName == "bows" || productName == "crossbows" || 
        productName == "leather armor" || productName == "maces" || 
        productName == "metal armor" || productName == "pikes" || 
        productName == "spears" || productName == "swords"
    );
    int quantity = isWeapon ? 1 : 5;


    ExecuteHumanMarketTrade(
        0,
        it->second,
        quantity
    );
}


// ============================================================
// Get Number Products
// ============================================================

int GameInterface::getNumberProducts(
    const std::string& productName
)
{
    // تحديث حالة إيقاف التجارة بشكل مستمر قبل استرجاع أي بيانات للتحقق من الأزرار
    UpdatePauseState();

    int playerId =
        GetPlayerId();


    if (
        playerId <= 0
    )
    {
        playerId = 1;
    }


    int inventoryIndex =
        playerId - 1;


    if (
        productBaseAddress == 0
    )
    {
        return 0;
    }


    const auto it =
        productIds.find(
            productName
        );


    if (
        it == productIds.end()
    )
    {
        return 0;
    }


    int realAmount =
        *reinterpret_cast<int*>(
            productBaseAddress
            +
            (
                0x39f4
                *
                inventoryIndex
            )
            +
            (
                (
                    0xe7d
                    +
                    it->second
                )
                *
                0x4
            )
        );


    ScanPlayersWood(
        productBaseAddress,
        marketIdAddress
    );


    // نمرر الرقم الحقيقي لدالة الذاكرة الافتراضية
    return GetVirtualEffectiveAmount(
        it->second,
        realAmount
    );
}


// ============================================================
// Available Products
// ============================================================

std::vector<std::string>
GameInterface::getAvailableProducts() const
{
    std::vector<std::string>
        availableProducts;


    for (
        const auto& pair :
        productIds
    )
    {
        availableProducts.push_back(
            pair.first
        );
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
    return HasMarket(
        GetPlayerId()
    );
}
