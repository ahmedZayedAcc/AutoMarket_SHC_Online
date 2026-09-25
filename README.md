AutoMarket_SHC_online

AutoMarket for Stronghold Crusader HD 1.3 with automatic Buy/Sell trading for offline and online multiplayer games.

Developer: Ahmed Zayed

Features

- Automatic Buy/Sell trading
- Offline and Online Multiplayer support
- Dynamic Player ID detection
- Support for all 8 players
- Dynamic inventory/storage detection
- Automatic market selection for the current player
- Configurable Buy/Sell thresholds
- Configurable trade frequency
- Resource trading
- Weapon trading
- Virtual inventory tracking
- Human market tracing
- Network market tracing
- Signature-based address detection
- In-game mouse-controlled menu
- DirectDraw proxy
- MinHook-based hooking
- Debug logging
- Session reset when leaving the game

Online Multiplayer

The project is designed to work with Stronghold Crusader HD 1.3 Online Multiplayer.

The DLL detects the local Player ID dynamically instead of assuming that the local player is always Player 1.

Player / Storage Mapping

Player ID| Storage / Inventory Index
Player 1| 0
Player 2| 1
Player 3| 2
Player 4| 3
Player 5| 4
Player 6| 5
Player 7| 6
Player 8| 7

The inventory index is calculated as:

inventoryIndex = playerId - 1;

Online Test

The project was tested with all 8 Player IDs.

During the tests:

- Player ID detection worked for Players 1–8.
- Inventory/Storage mapping worked correctly.
- No Player ID or Inventory ID recognition problems were observed.
- No "Connection Lost" occurred during the tests.

These points describe the tested configuration and are not intended as a guarantee for every possible network environment.

Trading

AutoMarket can automatically buy and sell configured products according to the configured thresholds.

Example:

Current amount > Sell threshold
    → Sell

Current amount < Buy threshold
    → Buy

Trading is performed for the currently detected local player.

Supported Weapons

The current configuration supports:

- Bows
- Crossbows
- Leather Armor
- Maces
- Metal Armor
- Pikes
- Spears
- Swords

Weapon trading uses the appropriate weapon quantity rules.

Supported Resources

The project also supports resource trading through the configured product list.

Resources use the game's market quantity rules rather than the weapon quantity rule.

Virtual Inventory Tracking

The project maintains a virtual inventory state to account for the delay between a market transaction and the corresponding update of the game's real inventory.

This prevents the AutoMarket logic from immediately treating the old inventory value as the current value after a trade.

The effective amount is calculated using:

Effective Inventory
    =
Real Inventory
    +
Virtual Trade Delta

The virtual state is updated when AutoMarket performs a Buy or Sell operation.

Dynamic Player Detection

The local Player ID is read dynamically from the game instead of being permanently hardcoded to Player 1.

The detected Player ID is then used to select the corresponding storage/inventory index:

int playerId = GetPlayerId();
int inventoryIndex = playerId - 1;

This allows the same DLL to operate according to the player currently running the DLL.

Dynamic Market Selection

The market address is selected using the current Player ID.

The project does not rely on a permanently selected player market for normal trading operations.

This is important for online multiplayer because each player process can have a different local Player ID.

Human Market Trace

The project includes tracing for the game's native market trading functions.

The trace can be used to observe:

- Buy operations
- Sell operations
- Product IDs
- Player-related market state
- Native market function calls

This was used during development to understand and verify the game's normal market behavior.

Network Market Trace

The project also includes network-level tracing for market-related commands.

The network trace can be used to monitor market commands sent by the game and inspect the associated state during online trading.

A specific market-related network command is also monitored by the debug system.

Controls

Numpad 1

Toggle the AutoMarket menu.

Numpad 1
    ↓
Show / Hide Menu

Numpad 2

Toggle automatic trading.

Numpad 2
    ↓
Trading ON / OFF

Mouse

The menu supports full mouse control.

The user can interact with the configuration directly through the in-game menu.

Save

The Save button saves the current configuration to:

automarket.ini

Configuration

The project uses configuration files to store AutoMarket settings.

Main configuration:

automarket.ini

Snapshot configuration:

automarketsave.ini

The configuration includes settings such as:

- Buy thresholds
- Sell thresholds
- Weapon count
- Trade frequency
- Menu settings
- Hotkeys
- UI settings

Trade Frequency

The trading interval can be configured through the configuration system.

Example:

TradeFrequency=50

The value controls how frequently AutoMarket checks and performs trading operations.

Session Reset

When the game leaves the active game state, AutoMarket resets its session-related state.

This prevents information from a previous game session from being incorrectly reused in a new session.

The reset includes the relevant trading and configuration state used during the active session.

Address Detection

The project uses signature-based address detection for important game functions and structures.

This avoids relying exclusively on fixed addresses for every internal function.

The initialization process detects and validates important addresses such as:

- Native market/trade function
- Market ID pointer
- Product base pointer
- Market action function

The project also uses known game offsets where appropriate.

DirectDraw Proxy

The DLL operates as a DirectDraw proxy.

The project exports the required DirectDraw functions and forwards them to the original system "ddraw.dll".

The proxy is also used as the entry point for installing the AutoMarket hooks.

MinHook

The project uses MinHook for function hooking.

Hooks are used for selected game and Windows functions required by AutoMarket.

The project initializes the hooks when the DLL is loaded and removes them during cleanup.

In-Game Menu

The AutoMarket menu is displayed inside the game and provides mouse-based configuration.

The menu can be opened and closed with:

Numpad 1

The menu allows the user to configure the trading behavior without manually editing every setting.

Debug Logging

Debug information is written to:

automarket_debug.log

The log can contain information related to:

- Address initialization
- Player ID changes
- Market detection
- Inventory detection
- Human market calls
- Network market calls
- Trading operations
- Session changes

The debug log is primarily intended for development and troubleshooting.

Trading Flow

The general trading flow is:

DLL Loaded
    ↓
Initialize Hooks
    ↓
Detect Game
    ↓
Detect Local Player ID
    ↓
Map Player ID → Storage Index
    ↓
Detect Current Market
    ↓
Read Inventory
    ↓
Apply Virtual Inventory State
    ↓
Check Buy/Sell Thresholds
    ↓
Execute Market Trade
    ↓
Update Virtual Inventory
    ↓
Continue Monitoring

Project Structure

The main components include:

AutoMarket_SHC_online/
│
├── dllmain.cpp
├── internal.cpp
├── internal.hpp
├── config.cpp
├── config.hpp
├── menu.cpp
├── menu.hpp
├── log.cpp
├── log.hpp
│
├── MinHook/
│
├── CMakeLists.txt
├── automarket.ini
└── automarketsave.ini

The exact project structure can change as development continues.

Requirements

Game

- Stronghold Crusader HD 1.3

Build Environment

The project can be built using:

- CMake
- MinGW-w64
- "i686-w64-mingw32"
- MinHook

The DLL is built as a 32-bit Windows DLL because Stronghold Crusader HD 1.3 is a 32-bit application.

Build

Example build flow:

mkdir build
cd build

cmake ..
cmake --build . --config Release

For a MinGW cross-compilation environment, the appropriate MinGW toolchain/compiler should be configured before building.

The resulting DLL is:

ddraw.dll

Installation

Place the compiled:

ddraw.dll

in the Stronghold Crusader game directory alongside the game executable.

Make sure the required configuration files are also available in the expected directory.

Important Notes

- The project targets Stronghold Crusader HD 1.3.
- Game version compatibility is important because internal addresses and signatures depend on the executable.
- Do not assume addresses from another game version are compatible.
- Debug logging can be enabled/used during development to investigate address, player, market, or network behavior.
- Online testing results are based on the tested Stronghold Crusader 1.3 environment.

Project Status

Current implementation includes:

- [x] Automatic Buy
- [x] Automatic Sell
- [x] Resource Trading
- [x] Weapon Trading
- [x] Dynamic Player ID
- [x] Player 1–8 Support
- [x] Player → Storage Mapping
- [x] Dynamic Inventory Detection
- [x] Dynamic Market Selection
- [x] Virtual Inventory Tracking
- [x] Human Market Trace
- [x] Network Market Trace
- [x] Online Multiplayer Testing
- [x] In-Game Menu
- [x] Mouse Control
- [x] Save Configuration
- [x] Pause / Resume Trading
- [x] Signature-Based Address Detection
- [x] DirectDraw Proxy
- [x] MinHook Integration
- [x] Debug Logging
- [x] Session Reset

Developer

Ahmed Zayed

AutoMarket_SHC_online is a Stronghold Crusader HD 1.3 AutoMarket project focused on automatic market trading, dynamic player/inventory detection, and online multiplayer support.
