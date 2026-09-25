AutoMarket_SHC_online

Developer: Ahmed Zayed

AutoMarket for Stronghold Crusader HD 1.3, designed for automatic Buy/Sell operations in both offline and online games.

---

Features

Online Multiplayer Support

- Supports all 8 players.
- Player IDs: 1–8.
- Storage / Inventory indexes: 0–7.
- Automatically uses the current player's data.

Player 1 → Storage 0
Player 2 → Storage 1
Player 3 → Storage 2
Player 4 → Storage 3
Player 5 → Storage 4
Player 6 → Storage 5
Player 7 → Storage 6
Player 8 → Storage 7

Dynamic Player ID

The current Player ID is read directly from the game.

The AutoMarket does not require manually selecting the player.

Dynamic Inventory Detection

The project accesses the inventory belonging to the detected player using the verified Player/Storage mapping.

Automatic Buy & Sell

Automatically buys or sells configured items according to the selected thresholds.

Example:

Current amount > Sell threshold
→ Sell

Current amount < Buy threshold
→ Buy

Resource Trading

Supports automatic trading of the configured resources.

Resource trades use the game's normal resource trading quantities.

Weapon Trading

Supports automatic trading for:

- Bows
- Crossbows
- Leather Armor
- Maces
- Metal Armor
- Pikes
- Spears
- Swords

Weapon trading uses the configured weapon quantity handling.

Configurable Trade Frequency

The time between automatic trading checks can be configured.

This prevents the AutoMarket from attempting to trade continuously on every frame.

Human Market Trace

Added tracing for the game's native market system.

The trace can monitor:

- Native trade function calls
- Buy / Sell operations
- Player slot
- Item ID
- Network market commands
- Relevant game memory values

This is mainly useful for debugging and analyzing multiplayer market behavior.

Network Market Trace

The project hooks the game's network send function and records market-related commands.

This allows the AutoMarket to monitor how market operations are sent through the game's multiplayer system.

Virtual Inventory Tracking

The project maintains virtual inventory changes after an automatic trade.

This helps account for the delay between sending a trade and seeing the updated quantity in the game's actual inventory.

Dynamic Market Selection

The market item is written to the current player's market-selection area before executing the Buy/Sell action.

This allows market operations to follow the currently detected player.

Signature-Based Address Detection

Important game addresses are resolved using signatures, including:

- Trade function
- Market ID
- Product / Inventory base
- Other required internal addresses

This reduces dependence on manually entering every address.

DirectDraw Proxy

The project works as a "ddraw.dll" proxy.

It forwards the original DirectDraw functions while providing the AutoMarket functionality.

Supported exports include:

DirectDrawCreate
DirectDrawCreateEx
DirectDrawEnumerateA
DirectDrawEnumerateW

MinHook

Uses MinHook for API and internal game-function hooks.

In-Game Menu

The project includes an in-game menu with mouse interaction for configuring the AutoMarket.

Configuration Files

The current configuration system uses:

automarket.ini
automarketsave.ini

Configuration includes:

- Buy thresholds
- Sell thresholds
- Trade frequency
- Weapon settings
- Menu settings
- Hotkeys

Pause System

Automatic trading can be paused without stopping the game.

Session Reset

AutoMarket state is reset when leaving the active game state so that previous session settings do not incorrectly carry into another game.

Debug Logging

The project includes logging for debugging and testing.

The log can contain information such as:

- Initialization
- Player ID changes
- Market addresses
- Trade operations
- Network commands
- Memory values
- Hook status

---

Player and Storage Mapping

The project uses two different indexes.

Player ID:

1–8

Storage / Inventory index:

0–7

Mapping:

Player| Storage
Player 1| 0
Player 2| 1
Player 3| 2
Player 4| 3
Player 5| 4
Player 6| 5
Player 7| 6
Player 8| 7

The inventory index is calculated as:

int inventoryIndex = playerId - 1;

---

Trading Flow

The automatic trading process is:

MyBlt()
   ↓
ExecuteTrade()
   ↓
Check game state
   ↓
Check trade frequency
   ↓
Read current inventory
   ↓
Compare thresholds
   ↓
Buy / Sell

---

Configuration Example

A typical item can have:

Sell Threshold = 500
Buy Threshold  = 100

The AutoMarket will:

Amount > 500
→ Sell

Amount < 100
→ Buy

---

Requirements

- Stronghold Crusader HD 1.3
- 32-bit game
- 32-bit "ddraw.dll"
- Windows / compatible Wine environment

For Building

- CMake
- MinGW "i686-w64-mingw32"
- MinHook
- C++ compiler

---

Build

The project builds a 32-bit:

ddraw.dll

The DLL is used as the DirectDraw proxy for Stronghold Crusader.

---

Project Status

AutoMarket_SHC_online is under development and testing for Stronghold Crusader HD 1.3, with a focus on automatic market trading and online multiplayer player/inventory handling.
