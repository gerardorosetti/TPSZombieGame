# TPS Zombie Survival - Proving Grounds

A production-grade, round-based Third-Person Shooter (TPS) Zombie Survival game built from the ground up with **Unreal Engine 5.8** and modern **C++**. Inspired by classic wave-survival arcade shooters (such as *Call of Duty: Zombies*), this project demonstrates clean, modular, and performant game architecture designed for both educational study and portfolio demonstration.

---

## 🎮 Features Overview

- **Modular Combat & Ballistics (`UCombatComponent`, `AWeaponBase`)**:
  - High-precision hitscan line-trace ballistics with bone-specific hit detection (differentiating headshots from body shots).
  - Procedural reload animation physics with detached magazine dropping and socket reattachment.
  - Smooth Aim Down Sights (ADS) camera interpolation and weapon spread dynamics.
  - Dual Camera Shakes: Recoil vibration on firing and directional impulse oscillation on player damage.
- **Perception-Driven Zombie Enemy AI (`AZombieEnemyBase`, `AZombieAIController`)**:
  - Hearing and Sight perception senses (`UAIPerceptionComponent`) driving state machine behaviors (Wander, Chase, Attack).
  - Dynamic friendly fire filtering and living-target verification (zombies cease attacking dead corpses immediately).
  - Round-based mathematical scaling of enemy health, movement speed, and attack rates.
  - Directional ragdoll physics impulses upon elimination.
- **Wave Lifecycle Subsystem (`AZombieWaveManager`, `AZombieSpawnPoint`)**:
  - Automated round progression with intermission timers, round-start stingers, and scaling zombie caps.
  - Dynamic spawn point discovery with visual zone gating.
- **Economy & World Interactables (`IInteractableInterface`)**:
  - **Obstacle Debris & Security Doors (`AObstacleDoor`)**: Multi-mesh physical barriers with real-time NavMesh obstruction cutting, unlockable via player score points.
  - **Wall-Buy Stations (`AWallBuyStation`)**: Interactive wall-mounted stations for ammo restocks and tactical power-ups.
  - **Power-Up Subsystem (`APowerUpBase`)**: Max Ammo, Insta-Kill (timed one-hit kill mode), Double Points (2x score multiplier), and Tactical Nuke (eliminates all living enemies and awards bonus points).
- **Audio & Niagara VFX Architecture**:
  - 3D spatialized Sound Attenuation across all gunshots, impacts, zombie screams, and environment doors.
  - Procedural low-health danger triad: Entry alarm stinger, looping heartbeat audio component during critical danger, and exit relief stinger upon recovery.
  - Niagara particle emitters: Muzzle flash aligned to weapon barrel, dynamic tracer beams, orienting blood splatters, surface dust/sparks, and persistent bullet hole decals.
- **Complete Frontend & Gameplay UI (UMG & Slate)**:
  - **Main Menu (`UMainMenuWidget`, `AMainMenuGameMode`)**: Automatic cine-camera actor viewport blending without requiring Level Blueprint nodes.
  - **Combat HUD (`UCombatHUDWidget`)**: Animated health bar, ammo indicators, low-health red screen vignette, hitmarkers with distinct headshot audio cues, and active power-up notification badges.
  - **Game Over Screen (`UGameOverWidget`)**: Full match summary analytics (rounds survived, total kills, headshot count, total score) and seamless game restart controls.

---

## 📋 Prerequisites

Before cloning and opening the project, ensure your workstation meets the following requirements:

1. **Operating System**: Windows 10 or 11 (64-bit).
2. **Unreal Engine**: **Unreal Engine 5.8** (Installed via the Epic Games Launcher).
3. **IDE / Compiler**: **Visual Studio 2022** (Community, Professional, or Enterprise):
   - In the Visual Studio Installer, ensure the **Game development with C++** workload is checked.
   - Individual components required:
     - *MSVC v143 - VS 2022 C++ x64/x86 build tools*
     - *Windows 10 SDK (10.0.18362.0 or higher) / Windows 11 SDK*
     - *Unreal Engine IDE Support*
4. **Version Control**: **Git** with **Git LFS (Large File Storage)** installed:
   - [Download Git](https://git-scm.com/)
   - [Download Git LFS](https://git-lfs.com/)

> [!IMPORTANT]
> This repository uses **Git LFS** to store 3D meshes, textures, audio files, and Unreal `.uasset` packages. You **must** install Git LFS before cloning, otherwise binary assets will be downloaded as tiny pointer files and the project will fail to load.

---

## 🚀 Step-by-Step Setup Guide (Clone & Play)

### Step 1: Initialize Git LFS
Open a terminal (PowerShell, Command Prompt, or Git Bash) and run:
```bash
git lfs install
```
*(You only need to run this command once on your computer).*

### Step 2: Clone the Repository
Clone the repository to your local drive:
```bash
git clone git@github.com:gerardorosetti/TPSZombieGame.git
```
*Or via HTTPS:*
```bash
git clone https://github.com/gerardorosetti/TPSZombieGame.git
```

Navigate into the project directory:
```bash
cd TPSZombieGame
```

Verify that all LFS binary assets are pulled correctly:
```bash
git lfs pull
```

### Step 3: Generate Visual Studio Project Files
1. Open the project root folder in Windows Explorer.
2. Right-click on `ISPPV1.uproject`.
3. Select **Generate Visual Studio project files**.
4. Unreal will inspect the C++ source files and generate the `ISPPV1.sln` file.

### Step 4: Compile the C++ Solution
1. Open `ISPPV1.sln` in **Visual Studio 2022**.
2. Set the build configuration drop-downs at the top:
   - Solution Configuration: **Development Editor**
   - Solution Platform: **Win64**
3. In the Solution Explorer, right-click on the `ISPPV1` project under `Games` and select **Build** (or press `Ctrl + Shift + B`).
4. Wait for the build to finish with `Build: 1 succeeded, 0 failed`.

### Step 5: Launch the Game in Unreal Editor
- **Option A (From Visual Studio)**: Press `F5` or `Ctrl + F5` to launch Unreal Editor with debugging attached.
- **Option B (Directly)**: Double-click `ISPPV1.uproject` to open the project in Unreal Editor 5.8.

Once inside the editor, simply click the green **Play** button (or press `Alt + P`) to start playing!

---

## 🕹️ Controls & Keybindings

| Action | Keyboard / Mouse Input | Description |
| :--- | :--- | :--- |
| **Move** | `W`, `A`, `S`, `D` | Character movement across the map |
| **Look / Turn** | `Mouse X / Y` | Free-camera 3rd person perspective control |
| **Fire Weapon** | `Left Mouse Button` | Fire equipped weapon (automatic / semi-auto) |
| **Aim Down Sights (ADS)** | `Right Mouse Button` (Hold) | Tighten camera over shoulder & reduce spread |
| **Reload** | `R` | Trigger procedural reload with physical mag drop |
| **Interact** | `E` | Clear door obstacles / Purchase from Wall-Buys |
| **Jump** | `Spacebar` | Jump over obstacles |

---

## 🗺️ Level Breakdown

- **`Lvl_MainMenu`** (`Content/ZombieGame/Levels/Lvl_MainMenu.umap`):
  - Game frontend entry point.
  - Automatically activates UI mouse controls, background menu music, and blends view to the level's cinematic camera actor without manual Blueprint code.
- **`Lvl_Default`** (`Content/ZombieGame/Levels/Lvl_Default.umap`):
  - Proving grounds military research facility survival arena.
  - Fully populated with NavMesh bounds, zombie spawn points, barricaded door obstacles, and Wall-Buy stations.

---

## 📦 Packaging a Standalone Executable (.exe)

The project configuration (`Config/DefaultGame.ini`) is already pre-configured to package both the main menu and the survival level into a self-contained `.exe`:

1. Open the project in **Unreal Editor 5.8**.
2. Click on the **Platforms** button in the top toolbar.
3. Hover over **Windows**:
   - Select **Binary Configuration** -> **Shipping** (for optimized performance) or **Development** (to retain console debugging commands via `~`).
   - Click **Package Project**.
4. Choose an output directory (e.g. `C:\Builds\ZombieGame`).
5. Unreal Engine will compile the game binaries, cook all assets, and generate a `Windows/` folder containing **`ISPPV1.exe`**.
6. You can zip and distribute the `Windows/` folder to anyone; they can run `ISPPV1.exe` directly with zero installations required!

---

## 🌳 Educational Milestone Branch History

This repository features clean step-by-step milestone branches showcasing the incremental development trajectory:

| Branch Name | Milestone Focus |
| :--- | :--- |
| `step-01-health-component` | Health subsystem, damage interfaces, and regeneration logic. |
| `step-02-weapon-system` | Modular combat component, hitscan ballistics, and procedural reload. |
| `step-03-zombie-enemy-ai` | AI perception, sight/damage senses, pathfinding, and melee behavior. |
| `step-04-wave-manager-and-scaling` | Wave spawner, round intermission, and health/speed scaling math. |
| `step-05-economy-interactables-powerups-and-hud` | Wall buys, debris doors, power-ups, and in-game combat HUD. |
| `step-06-game-over-audio-and-polish` | Spatial audio cues, Niagara VFX, game over lifecycle, and main menu. |
| **`main`** | **Fully integrated, complete game with all systems unified.** |

---

## 📄 License

Academic / Educational Project - Developed in 2026. All rights reserved.
Meshes and audio assets belong to their respective creators under standard Epic Games / Fab marketplace licenses.
