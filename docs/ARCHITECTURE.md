# Architecture

## Goals

- Content is data. Levels, cutscenes, menus, prefabs and input live in JSON. The game and the editor read the same files through the same code.
- The rules live in one place. One schema describes every object type, component, action, scene setting and camera property. The inspector, validator, serializer and runtime all read it, so they cannot disagree.
- Testable without the engine. Everything that is not rendering or platform glue lives in `Core`, which is plain C++20 and runs under unit tests in CI.
- Safe editing. Every change goes through a command, so undo/redo, dirty tracking and live view updates all work the same way.

## Layers

```text
 ┌──────────────┐  ┌──────────────────┐  ┌─────────┐
 │ Game         │  │ Editor (ImGui)   │  │ hg_tool │
 └──────┬───────┘  └────────┬─────────┘  └────┬────┘
        │                   │                 │
 ┌──────▼───────────────────▼──────┐          │
 │ Runtime (Axmol)                 │          │
 └──────────────┬──────────────────┘          │
                │                             │
 ┌──────────────▼─────────────────────────────▼──────┐
 │ Core (C++20, rapidjson only)                       │
 └────────────────────────────────────────────────────┘
```

### Core (`Source/Core`)

| Module | Contents |
| --- | --- |
| `Base` | `Value` (null/bool/int/double/string/array/ordered object), JSON parse/write (comments and trailing commas accepted, deterministic output), `Diagnostics`, `Result`, math types and value converters |
| `Scene` | `SchemaRegistry` and the built-in schema, `SceneDocument`/`ObjectDesc` with stable uids, the serializer, `PrefabLibrary` and prefab expansion, `Validator` |
| `Edit` | `Command`, `CompositeCommand`, `CommandHistory` (merge window, saved-state tracking), the scene commands, `EditSession`, `ChangeSet` |
| `Animation` | Easing presets and cubic Bézier, `PathShape`/`PathSampler` (linear, Catmull-Rom, Bézier, arc-length parameterised), `TimelineDesc` and the evaluator |
| `Gameplay` | `CharacterMotor`, a kinematic AABB mover for both control modes (gravity, coyote time, jump buffering, jump cut, one-way platforms in side-scroll; free analog movement in top-down) |
| `App` | `AppConfig` and the input binding model |

### Runtime (`Source/Runtime`)

| Module | Contents |
| --- | --- |
| `App` | `AppContext` (owns the services below), `ContentStore` (file access, writable root for the editor), `InputService` (action map, keyboard, touch stick), `AudioService`, `Log` |
| `Scene` | `NodeFactory` (type creators plus one applier per property), `BuiltinNodes`, `SceneView` (uid → `ax::Node` map, camera transform, coordinate conversions, parallax) |
| `Gameplay` | `World` (level state: player, triggers, followers, solids, Y-sort, actions, timelines) and `CameraRig` (dead zone, damping, timeline overrides with blend-out, shake, world clamp) |
| `Animation` | `TimelinePlayer`, which drives a `TimelineTarget` |
| `Scenes` | `PlayScene`, the `ax::Scene` that loads a document and runs a `World` |

### Applications

- `Source/App`: the shared `AppDelegate` and window setup (design resolution with a fixed height, so wider screens see more of the level).
- `Source/Game`: starts `PlayScene` with `start_scene` from `app.json`.
- `Source/Editor`: `EditorScene` hosts `EditorApp`, which owns the open document, the `SceneView` and the panels.

## Data flow

### Loading a level

```text
ContentStore.loadScene(id)
  → readScene (JSON → SceneDocument, version check)
  → expandPrefabs (instances merged with their prefab, child uids combined)
  → SceneView.build (NodeFactory creates nodes, applies every property)
  → World.start (components → player/followers, triggers, solids, camera, on_start actions)
```

### A frame of play

```text
World.update
  updatePlayer      input → MotorInput → stepMotor → position
  updateFollowers   PathFollower components sample their path
  updateTimelines   TimelinePlayer → TimelineTarget (properties, camera, events)
  updateTriggers    AABB overlap → on_enter / on_exit actions
  updateSorting     y_sort children reordered by Y
  updateCamera      CameraRig follows the player and blends timeline overrides
```

Actions such as `change_scene` and `quit_game` are deferred to the end of the frame, so a trigger cannot destroy the world it is running in.

### Editing

```text
Panel → EditorApp.execute(Command)
  → EditSession → CommandHistory.apply → SceneDocument mutated
  → ChangeSet → EditorApp.onDocumentChanged
       property changes → SceneView.applyProperty (patched in place)
       structural changes → SceneView rebuilt
```

The document is the only source of truth. The view is disposable and never written back. Gizmo drags use the merge window, so one drag becomes one undo step. Timeline record mode turns a property edit into a key at the playhead.

### Previewing timelines

`EditorApp` implements `TimelineTarget`, so a preview drives the editor's `SceneView` and a virtual camera through exactly the same evaluator as the game. Scrubbing calls `applyAt(time)`. Stopping the preview restores document values.

### Play in editor

`startPlay` saves a copy of the document, creates a `PlayScene` from it (`loadDocument`, not from disk) and pushes it over the editor scene. The overlay offers pause, step, restart, time scale and debug draw. `Esc` pops the scene. The edited document is never touched, so unsaved work survives a play session.

## Scene format

```text
SceneDocument
  format, version, id, kind (location | screen)
  settings      world_size, view_height, background, control_mode, camera_*, music, on_start
  objects[]     uid, name, type, prefab?, props{}, components[], children[]
  timelines[]   id, duration, loop, tracks[]
                  property: target, property, keys[{time, value, ease}]
                  path:     target, path, start, duration, ease, orient
                  event:    events[{time, action}]
  extra         unknown keys, written back unchanged
```

Object references (`ObjectRef`) and path references store uids, never names. `hg_tool validate` and the editor's validator check types, ranges, enums, references, assets, scene ids and action parameters.

## Adding features

| Feature | Core | Runtime |
| --- | --- | --- |
| Object type | `addType` in `BuiltinSchemas.cpp` | `registerType` / `registerProperty` in `BuiltinNodes.cpp` |
| Property on an existing type | add it to the type's schema | `registerProperty` |
| Component | `addComponent` | spawn handling and update in `World` |
| Action | `addAction` | a branch in `World::runAction` |
| Scene setting | append to `setSceneSettings` | read it in `World::start` or `SceneView` |
| Camera property | append to `setCameraProperties` | handle it in `World::applyCameraProperty` and `CameraRig` |

The editor needs no changes for any of these: inspector widgets, pickers, validation and timeline tracks are generated from the schema.

## Platform notes

- Android and iOS get the game only. The editor is a desktop target and uses `ContentStore::setWritableRoot` to save straight into `Content/` in the source tree.
- `axmol/` is pinned to v2.11.5. `cmake/modules/AXGameEngineOptions.cmake` turns off unused engine modules (3D, physics, Lua, spine and others) to keep build times and binary size down.
