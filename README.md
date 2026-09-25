AutoMarket_SHC_online

Developer: Ahmed Zayed

AutoMarket for Stronghold Crusader HD 1.3, designed for automatic Buy/Sell operations in offline and online games.

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

No manual player selection is required.

Dynamic Inventory Detection

The project accesses the inventory belonging to the detected player using the verified Player/Storage mapping.

Automatic Buy & Sell

Automatically buys or sells configured items according to the selected thresholds.

Amount > Sell Threshold
→ Sell

Amount < Buy Threshold
→ Buy

Resource Trading

Supports automatic trading of the configured resources.

Weapon Trading

Supports automatic trading of:

- Bows
- Crossbows
- Leather Armor
- Maces
- Metal Armor
- Pikes
- Spears
- Swords

Configurable Trade Frequency

The time between automatic trading checks can be configured.

---

Controls

Numpad 1 — Toggle Menu

Press:

Numpad 1

to open or close the AutoMarket menu.

Numpad 2 — Toggle Trading

Press:

Numpad 2

to toggle automatic trading:

Numpad 2
    ↓
Pause Trading
    ↕
Resume Trading

The game continues running normally while AutoMarket trading is paused.

Full Mouse Control

The menu is fully controllable with the mouse.

The mouse can be used to:

- Navigate the menu
- Select items
- Change values
- Configure Buy thresholds
- Configure Sell thresholds
- Control the AutoMarket settings

No keyboard-only menu navigation is required.

Save Settings

The menu includes a Save control for saving the current settings to:

automarket.ini

Saved settings can be loaded again when the DLL starts.

---

Configuration

The project uses:

automarket.ini
automarketsave.ini

The configuration system supports settings such as:

- Buy thresholds
- Sell thresholds
- Trade frequency
- Weapon settings
- Menu settings
- Hotkeys

---

Human Market Trace

The project includes tracing for the game's native market system.

The trace can monitor:

- Native trade function calls
- Buy / Sell operations
- Player ID
- Item ID
- Network market commands
- Relevant game memory values

This is useful for debugging and testing multiplayer market behavior.

---

Network Market Trace

The project hooks the game's network send function and records market-related commands.

This allows market operations to be monitored through the game's multiplayer network layer.

---

Virtual Inventory Tracking

The AutoMarket maintains virtual inventory changes after an automatic trade.

This helps handle the delay between executing a trade and receiving the corresponding inventory update from the game.

---

Dynamic Market Selection

Before executing a Buy/Sell operation, the project writes the selected item to the market-selection area belonging to the current player.

The market operation is then executed using the game's internal market action.

---

Signature-Based Address Detection

Important game addresses are resolved using signatures, including:

- Trade function
- Market ID
- Product / Inventory base
- Required internal addresses

---

DirectDraw Proxy

The project works as a "ddraw.dll" proxy.

It forwards the original DirectDraw functions while providing the AutoMarket functionality.

Supported exports include:

DirectDrawCreate
DirectDrawCreateEx
DirectDrawEnumerateA
DirectDrawEnumerateW

---

MinHook

Uses MinHook for API and internal game-function hooks.

---

In-Game Menu

The AutoMarket menu is rendered inside the game through the DirectDraw rendering path.

It provides mouse-based configuration of the trading system.

---

Session Reset

AutoMarket state is reset when leaving the active game state.

This prevents previous session state from being incorrectly carried into another game.

---

Debug Logging

The project includes logging for debugging and testing.

The log can contain:

- Initialization information
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

Storage / Inventory Index:

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

MyBlt()
   ↓
ExecuteTrade()
   ↓
Check Game State
   ↓
Check Trade Frequency
   ↓
Read Inventory
   ↓
Compare Thresholds
   ↓
Buy / Sell

---

Requirements

- Stronghold Crusader HD 1.3
- 32-bit game
- 32-bit "ddraw.dll"
- Windows / compatible Wine environment

Building

- CMake
- MinGW "i686-w64-mingw32"
- MinHook
- C++ compiler

---

Build Output

The project builds:

ddraw.dll

as a 32-bit DirectDraw proxy for Stronghold Crusader HD 1.3.

---

Project Status

AutoMarket_SHC_online is under development and testing for Stronghold Crusader HD 1.3, with a focus on automatic market trading, online multiplayer support, dynamic player/inventory handling, and mouse-controlled configuration.
