# Horror Game

A 2D horror game that switches between **side-scroll** and **top-down** control on the fly, built on [Axmol](https://github.com/axmolengine/axmol) 2.11 (a maintained cocos2d-x 4 fork) with a data-driven content pipeline and an in-house level editor.

Target platforms: Windows, Linux and Android. macOS and iOS project files are kept in working order for later.

---

## Layout

```text
Content/             Game data shipped with the app (replaces the old Resources/)
  config/app.json    Window, start scene, preloaded atlases, input map
  scenes/*.json      Scenes and levels ("horror-scene" v2)
  prefabs/*.json     Reusable object trees ("horror-prefab")
  res/, fonts/       Textures, atlases, fonts

Source/
  Core/              Engine-independent C++20: values, JSON, schema, documents,
                     validation, commands/undo, easing, paths, timelines, motor
  Runtime/           Axmol layer: content store, input, audio, node factory,
                     SceneView, World, camera rig, timeline player, PlayScene
  App/               Shared AppDelegate and window bootstrap
  Game/              Game executable entry
  Editor/            ImGui level editor (desktop only)

Tools/hg_tool/       Command line: validate, format, import-legacy
Tests/               doctest unit tests for Core and shipped content
axmol/               Engine submodule (v2.11.5)
legacy/              The previous cocos2d-x sources and configs, kept for reference
```

Dependencies only point downwards: `Editor`/`Game` → `Runtime` → `Core`. `Core` never includes engine headers, so it builds and tests without Axmol in seconds (`HG_CORE_ONLY=ON`).

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full picture.

---

## Building

### Prerequisites

- CMake 3.22+, Ninja, a C++20 compiler (MSVC 2022, Clang 15+, GCC 12+)
- PowerShell 7 (Axmol uses it to sync resources and fetch the `axslcc` shader compiler)

```sh
git clone --recursive https://github.com/ConstantineManychev/HorrorGame.git
cd HorrorGame
pwsh axmol/setup.ps1
```

`setup.ps1` installs system packages on Linux and registers the `axmol` command line tool. Restart the terminal afterwards.

### Game and editor

```sh
axmol build -p linux -a x64 -xc '-Bbuild/linux'
axmol build -p win32 -a x64 -xc '-Bbuild/win32'
axmol build -p android -a arm64
```

Plain CMake works too once `axslcc` is on `PATH`:

```sh
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux --target HorrorGame HorrorGameEditor
```

| Target | Description |
| --- | --- |
| `HorrorGame` | The game |
| `HorrorGameEditor` | Level editor, desktop only (`HG_BUILD_EDITOR`, on by default) |
| `hg_tool` | Content tool |
| `hg_tests` | Unit tests |

### Core only (no engine, fast)

```sh
cmake -S . -B build/core -G Ninja -DHG_CORE_ONLY=ON -DHG_WARNINGS_AS_ERRORS=ON
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

### CMake options

| Option | Default | Meaning |
| --- | --- | --- |
| `HG_CORE_ONLY` | `OFF` | Build only `hg_core`, `hg_tool` and `hg_tests` |
| `HG_BUILD_EDITOR` | `ON` | Build the editor on desktop platforms |
| `HG_BUILD_TESTS` | `ON` | Build unit tests |
| `HG_WARNINGS_AS_ERRORS` | `OFF` | `-Werror` / `/WX` for project code |

CI (`.github/workflows/ci.yml`) checks formatting, builds and tests the core with GCC, Clang and MSVC, and builds the game and editor on Linux.

---

## Editor

The editor edits the files in `Content/` of the source tree directly, so saved changes go straight into git.

Panels: **Scenes**, **Hierarchy**, **Inspector**, **Viewport**, **Timeline**, **Assets**, **Console**. `View → Reset Layout` restores the default docking.

### Working with objects

- Create objects from `Create`, the hierarchy context menu, or by dragging a texture, atlas frame or prefab from **Assets** into the viewport.
- Reparent and reorder by dragging in the hierarchy. `F2` renames.
- The inspector edits every property described by the schema, adds and removes components, and edits scene settings when nothing is selected. Asset fields accept drops from **Assets**.
- Every change is a command: undo and redo cover creation, deletion, reparenting, properties, components, settings and timelines. Continuous drags merge into one undo step.
- `File → Validate Scene` (or the button in **Console**) reports broken references, missing assets and wrong value types.

### Viewport

| Input | Action |
| --- | --- |
| LMB | Select, move selection, drag gizmo |
| LMB on empty space | Box select |
| Ctrl + LMB | Add to or remove from selection |
| MMB / RMB / Space + LMB | Pan |
| Wheel | Zoom at the cursor |
| Ctrl while dragging | Toggle grid snapping |
| Arrows / Shift + Arrows | Nudge by 1 / 10 units |
| Ctrl + LMB on a selected path | Append a point |
| Alt + LMB on a path point | Remove the point |

`View → Game Camera Frame` outlines what the game camera sees at the current preview time, and **Camera view** in the timeline looks through it. Helpers draw triggers, colliders, spawn points and paths.

### Timelines

A timeline holds tracks that target an object or the camera (`@camera`):

- **Property tracks** animate any numeric, vector or color property: position, scale, rotation, opacity, color, camera center, zoom, rotation, shake and so on. Each key has an ease: linear, step, sine/quad/cubic/expo/back in-out variants, elastic and bounce out, or a custom cubic Bézier.
- **Path tracks** move an object or the camera along a `Path` object (linear, Catmull-Rom or Bézier with handles), at constant speed, with optional orientation along the path.
- **Event tracks** fire actions at a given time, such as `change_scene`, `play_sound`, `set_control_mode` or `play_timeline`.

Scrub the ruler to preview, press Play to watch in the viewport, drag keys and clips to retime them (Shift disables 0.05 s snapping), and right-click a key to change its ease. With **Record** on, edits in the viewport or inspector write keys at the playhead.

### Play in editor

`Ctrl+P` runs the open scene from its spawn point. `Ctrl+Shift+P` spawns the player at the viewport center. The overlay offers Pause, Step, Restart, time scale and debug draw. `Esc` returns to the editor with the document untouched.

### Shortcuts

| Keys | Action |
| --- | --- |
| Ctrl+S | Save scene |
| Ctrl+Z / Ctrl+Y, Ctrl+Shift+Z | Undo / redo |
| Ctrl+C / Ctrl+V / Ctrl+D | Copy / paste / duplicate |
| Del | Delete selection |
| F / Home | Frame selection / frame all |
| W / E / R | Move / rotate / scale gizmo |
| Ctrl+P / Ctrl+Shift+P | Play / play from viewport center |

---

## Game controls

The `input` map in `Content/config/app.json` defines the controls. The defaults are listed below. On touch devices a virtual stick and jump button are added.

| Action | Keys |
| --- | --- |
| Move | WASD / arrows |
| Jump | Space |
| Use | E |
| Run | Shift |
| Pause / back | Esc |

---

## Content format

Scenes are JSON documents with stable object `uid`s, so references survive renames and reordering:

```json
{
	"format": "horror-scene",
	"version": 2,
	"id": "stage_a",
	"kind": "location",
	"settings": { "control_mode": "top_down", "view_height": 1600, "on_start": [{ "do": "play_timeline", "timeline": "intro" }] },
	"objects": [
		{ "uid": 3, "name": "PlayerStart", "type": "SpawnPoint", "props": { "prefab": "prefabs/player.json", "position": [1900, 200] } },
		{ "uid": 7, "name": "ToSideScroll", "type": "Trigger", "props": { "position": [3700, 1000], "size": [280, 2000], "on_enter": [{ "do": "set_control_mode", "mode": "side_scroll" }] } }
	],
	"timelines": [ ... ]
}
```

- Objects form a tree through `children`. A `prefab` field instantiates a prefab and `props` override its values.
- Behaviour comes from components: `CharacterBody`, `PlayerController` and `PathFollower`.
- Unknown keys are preserved on save, so newer data survives older tools.
- The writer is deterministic (tabs, stable key order, compact scalar arrays), which keeps diffs small.

### hg_tool

```sh
hg_tool validate Content                            # schema, references, assets; non-zero exit on errors
hg_tool format Content/scenes/*.json                # rewrite files in canonical form
hg_tool import-legacy legacy Content                # convert old cocos2d-x configs to format v2
```

---

## Extending

**A new object type**

1. Describe it in `Source/Core/Scene/BuiltinSchemas.cpp` with `addType`: its base type, properties, defaults, ranges and asset kinds. The inspector, validator and serializer pick it up from there.
2. Bind it in `Source/Runtime/Scene/BuiltinNodes.cpp` with `registerType` (creates the `ax::Node`) and `registerProperty` (applies each value). `NodeFactory::findUnboundProperties` lists schema properties that still lack a binding.

**A new action**

1. Add it with `addAction` in `BuiltinSchemas.cpp` so editors and the validator know its parameters.
2. Handle it in `World::runAction` (`Source/Runtime/Gameplay/World.cpp`).

**A new component**

1. Add it with `addComponent` in `BuiltinSchemas.cpp`.
2. Read it in `World` when objects are spawned and drive it from `World::update`.

Pure logic such as motion, easing and evaluation belongs in `Core` and gets a unit test in `Tests/`.

---

## Code style

`.clang-format` and `.editorconfig` define the style: Allman braces, 4 spaces, no comments in favour of descriptive names. The CI format job fails on any deviation:

```sh
git ls-files 'Source/*.cpp' 'Source/*.h' 'Tests/*.cpp' 'Tools/*.cpp' 'Tools/*.h' | xargs clang-format -i
```

---

## Legacy

`legacy/` holds the previous cocos2d-x 4 implementation (`Classes/`, `configs/`, platform projects). Nothing builds from it. It stays only as a source for `hg_tool import-legacy` and can be deleted once everything has been migrated.
