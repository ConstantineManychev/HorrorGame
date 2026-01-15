# Horror Game 🛡️

**Horror Game** is a 2D platformer architected to demonstrate scalable game development patterns in C++. While built on the **Cocos2d-x** framework, the core focus of this project is to showcase a **data-driven**, **component-based** architecture that prioritizes loose coupling and maintainability.

---

## 🏗️ Architectural Highlights

This project moves away from simple inheritance chains, favoring **Composition over Inheritance** and strictly separated logic layers.

### 1. Entity-Component System (ECS) Approach
Instead of a monolithic `Player` class, game entities are constructed dynamically using components. This allows for flexible behavior attachment at runtime.
* **Components:** `PhysicsBodyComponent`, `InputComponent`, `SpriteComponent`, `LightComponent`.
* **Systems:** `LightingSystem`, `CameraSystem` handle the logic processing for specific components.

### 2. Design Patterns Implementation
The codebase demonstrates practical application of classic GoF and Game Programming patterns:
* **Event Bus:** Decouples game systems. Systems communicate via events (e.g., collisions, game state changes) without knowing about each other.
* **Service Locator:** Provides a global access point to services like Audio and Input managers without passing references everywhere.
* **Factory Pattern:** `EntityFactory` and `ViewFactory` encapsulate the complex creation logic of entities and UI elements.
* **Singleton:** Used for major Managers (`GameDirector`, `DataManager`) to ensure single instances for global state control.

### 3. Data-Driven Design
Hardcoding values is minimized. The game relies heavily on external configuration files, making it easier for designers to tweak the game without recompilation.
* **JSON Configs:** Levels, UI layouts (`views_list.json`), and entity parameters are loaded from `Resources/configs/`.

---

## 🛠️ Tech Stack

* **Language:** C++ (C++14/17 standards)
* **Engine:** Cocos2d-x v4
* **Build System:** CMake
* **Physics:** Box2D integration via components
* **Data Serialization:** JSON (RapidJSON)
* **Platforms:** Cross-platform support for Mobile (iOS, Android) and Desktop (Windows/Linux).

---

## 📂 Project Structure

```text
Classes/
├── Basics/          # Core base classes (BaseScene, GameEntity, ServiceLocator)
├── Components/      # ECS Components (Physics, Input, Render)
├── Factories/       # Entity and View creation logic
├── Logics/          # Game rules and logic solvers (e.g. GestureRecognizer)
├── Managers/        # Global systems (EventBus, DataManager, ScenesManager)
├── Scenes/          # High-level scene flow (MainGameScene, EditorScene)
├── Systems/         # Update loops for specific domains (Lighting, Camera)
└── Types/           # Common type definitions and Enums
Resources/
└── configs/         # JSON configuration files (Data-Driven core)
```
---

## 🚀 Build Instructions

This project uses **Git Submodules** to manage the Cocos2d-x engine dependency.

### Prerequisites
* CMake 3.10+
* C/C++ Compiler (MSVC, GCC, or Clang)
* Python 2.7+ (required for Cocos dependency setup)

### Setup & Build

1.  **Clone the repository with submodules:**
    ```bash
    git clone --recursive https://github.com/ConstantineManychev/HorrorGame.git
    cd HorrorGame
    ```
    *(If you already cloned without `--recursive`, run `git submodule update --init --recursive`)*

2.  **Download Engine Dependencies:**
    Cocos2d-x requires external binaries (curl, openssl, etc.) to be downloaded once.
    ```bash
    cd cocos2d
    python download-deps.py -r yes
    cd ..
    ```

3.  **Build with CMake:**
    ```bash
    mkdir build && cd build
    cmake ..
    cmake --build .
    ```
