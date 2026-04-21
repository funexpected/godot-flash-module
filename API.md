# Flash module — GDScript API

Reference for every class this module registers with `ClassDB`. For runtime internals see [HOWITWORKS.md](HOWITWORKS.md); for import-time and on-disk layout see [DATAMODEL.md](DATAMODEL.md).

All classes are registered in [register_types.cpp:103-122](register_types.cpp#L103-L122).

---

## Glossary

The flash module uses a handful of nested concepts. Keep this table handy when reading the rest of the doc.

| Term | Meaning |
|------|---------|
| **Symbol** | A named `FlashTimeline` living under `FlashDocument.symbols`. Picked on a `FlashPlayer` via `active_symbol`. The special value `""` means "the document's main timeline". |
| **Timeline** | The container holding layers + frames for one symbol (or the main timeline). |
| **Layer** | A z-ordered stack of frames within a timeline. Layers may be `type = "mask"` (they mask lower layers) or plain. |
| **Frame label** | A name placed on a keyframe. `label_type ∈ {"name", "anchor", "comment"}`. |
| **Clip** | A named frame range inside a symbol, delimited by `"name"` labels. Picked on a `FlashPlayer` via `active_clip`. |
| **Track** | A layer whose name you can address from the outside — typically one that carries clip labels. `get_clips_tracks()` returns the list; `advance_clip_for_track()` drives one independently. Different from `active_clip`, which is timeline-wide. |
| **Variant** | A skinning override. Keyed as `variants[layer_name][variant_name][symbol_token] = frame_index`. Selected via `set_variant(layer_name, variant_name)`. |
| **Animation event** | Runtime signal emitted when a `"comment"`-labeled frame is crossed. Delivered as `animation_event(name: String)`. |

**`active_symbol` vs `active_clip`.** `active_symbol` picks *which timeline* plays. `active_clip` picks a *named sub-range* within that timeline. Changing `active_symbol` resets `active_clip` to `""` ([flash_player.cpp:818-824](flash_player.cpp#L818-L824)), so set the symbol first, then the clip.

---

## `FlashPlayer` — Node2D

The scene node you actually instance. All properties/methods below are bound in [`_bind_methods()`](flash_player.cpp#L734-L811).

### Properties

| Property | Type | Notes |
|----------|------|-------|
| `playing` | `bool` | Drives `_notification(PROCESS)`. |
| `loop` | `bool` | Loop at `playback_end`. If `false`, emits `animation_completed` on reaching the end and clamps. |
| `frame_rate` | `float` | Default 24. Converts seconds → frames in `advance()`. |
| `resource` | `FlashDocument` | The imported document. Setting this re-resolves `active_symbol` and emits `resource_changed`. |
| `active_symbol` | `String` | Enum — keys of `FlashDocument.symbols` plus `""` for the main timeline. |
| `active_clip` | `String` | Enum — clip labels within the active symbol. |
| `replace_texture` | `Texture` | Optional overlay blit into the shader's `OVERLAY_TEXTURE` uniform. |
| `render_mode` | `RenderMode` | `RENDER_NORMAL` (default) or `RENDER_METABALL`. |
| `metaball/color` | `Color` | Tint when `render_mode = RENDER_METABALL`. |
| `metaball/balancing` | `WeightBalancing` | `WEIGHT_BALANCE_NONE` / `LINEAR` / `EXPONENTIAL`. |
| `metaball/threshold` | `float` (0.1..3.0) | Cutoff for the distance-sum metaball shader. |
| `metaball/debug` | `bool` | Shader-side debug visualization. |

Deprecated aliases (hidden from the inspector but still work in scripts):

- `active_label` → same as `active_clip` ([flash_player.cpp:798-803](flash_player.cpp#L798-L803))
- `active_timeline` → same as `active_symbol`

### Methods

| Method | Signature | Notes |
|--------|-----------|-------|
| `set_playing` / `is_playing` | `(bool) / () → bool` | Playing flag. |
| `set_loop` / `is_loop` | `(bool) / () → bool` | Loop flag. |
| `set_frame_rate` / `get_frame_rate` | `(float) / () → float` | FPS. |
| `set_frame` / `get_frame` | `(float) / () → float` | Current playback frame (float). Calling `set_frame(0)` resets to start — common reset idiom. |
| `override_frame` | `(symbol: String, frame: Variant)` | Forces a specific symbol to render at a pinned frame, bypassing its own playhead. Pass `null`/`NIL` to clear. |
| `set_resource` / `get_resource` | `(FlashDocument) / () → FlashDocument` | Swaps documents. Emits `resource_changed`. |
| `get_duration` | `(symbol = "", clip = "") → float` | Duration in frames. If both empty, returns the main timeline's length. |
| `set_active_symbol` / `get_active_symbol` | `(String) / () → String` | Switches timeline; resets `active_clip` to `""`. Emits `set_active_symbol_called`. |
| `set_active_clip` / `get_active_clip` | `(String) / () → String` | Picks a named sub-range within the active symbol. Emits `set_active_clip_called`. |
| `set_variant` / `get_variant` | `(key: String, value) / (key) → String` | Skinning — see glossary. |
| `get_variants` | `() → Dictionary` | Current per-layer variant selections. |
| `set_overlay_texture` / `get_overlay_texture` | `(Texture) / () → Texture` | Optional overlay (`replace_texture` property). |
| `advance` | `(time: float, seek: bool = false, advance_all_tracks: bool = false)` | Manual step. If `seek = true`, `time` is an absolute frame-offset from `playback_start`. |
| `advance_clip_for_track` | `(track: String, clip: String, time: float, seek: bool)` | Drives one named track independently. Used by `AnimationNodeFlashClip` for multi-track trees. |
| `get_symbols` | `() → PoolStringArray` | All symbol tokens in the document. |
| `get_clips` | `(symbol = "") → PoolStringArray` | Clip labels within `symbol` (or the active one). |
| `get_clips_tracks` | `() → PoolStringArray` | Named layers (tracks) exposing clips. |
| `get_clips_for_track` | `(track: String) → PoolStringArray` | Clip labels scoped to one track. |
| `get_clip_duration` | `(track: String, clip: String) → float` | Track-scoped duration. |
| `set_render_mode` / `get_render_mode` | `(RenderMode) / () → RenderMode` | Normal or metaball. |
| `set_metaball_threshold` / `get_metaball_threshold` | `(float) / () → float` | |
| `set_metaball_color` / `get_metaball_color` | `(Color) / () → Color` | |
| `set_metaball_debug` / `is_metaball_debug` | `(bool) / () → bool` | |
| `set_metaball_weight_balancing` / `get_metaball_weight_balancing` | `(WeightBalancing) / () → WeightBalancing` | |

### Signals

Bound at [flash_player.cpp:788-795](flash_player.cpp#L788-L795):

| Signal | Payload | When |
|--------|---------|------|
| `resource_changed` | — | `set_resource()` was called. |
| `animation_completed` | — | A clip/symbol reached its end. Deferred. In editor (`is_editor_hint`) it's suppressed. |
| `animation_event` | `name: String` | A `"comment"`-labeled frame was crossed. Deferred. |
| `advance_called` | `time, seek, advance_all_frames` | Emitted from inside `advance()`. Useful for wrapper/proxy players (see *Proxy player* pattern below). |
| `advance_clip_for_track_called` | `track, clip, time, seek` | Emitted from inside `advance_clip_for_track()`. |
| `set_active_symbol_called` | `value: String` | Emitted from inside `set_active_symbol()`. |
| `set_active_clip_called` | `value: String` | Emitted from inside `set_active_clip()`. |

### Enums

```gdscript
enum RenderMode { RENDER_NORMAL, RENDER_METABALL }
enum WeightBalancing { WEIGHT_BALANCE_NONE, WEIGHT_BALANCE_LINEAR, WEIGHT_BALANCE_EXPONENTIAL }
```

---

## `FlashDocument` — Resource

The imported artifact. You load it with `preload()` or `load()` like any `Resource`. Most of its API is for the module itself — scripts mainly touch `load_file`, `get_duration`, and `get_variants`.

| Member | Kind | Notes |
|--------|------|-------|
| `load_file(path: String) → Error` | method | Rarely used from GDScript; importer calls this. |
| `get_duration(symbol = "", label = "") → float` | method | Duration in frames. |
| `get_variants() → Dictionary` | method | Nested variant dictionary (see DATAMODEL.md). |
| `atlas` | property | `Ref<TextureArray>` — internal. |
| `symbols` | property | `Dictionary` keyed by symbol token — internal. |
| `bitmaps` | property | `Dictionary` keyed by bitmap name — internal. |
| `timelines` | property | `Array` of top-level timelines — internal. |

The resource is effectively immutable after import — don't mutate `symbols` or `atlas` from scripts.

---

## Internal resource classes

`FlashTimeline`, `FlashLayer`, `FlashFrame`, `FlashDrawing`, `FlashInstance`, `FlashBitmapInstance`, `FlashGroup`, `FlashShape`, `FlashTween`, `FlashTextureRect`, `FlashBitmapItem` are all registered so editors can inspect them, but their properties are flagged `PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL`. **Don't script against them.** Reach them via `FlashDocument.symbols` only if you're doing tooling — the normal game API goes through `FlashPlayer`.

Full field lists live in [DATAMODEL.md § Per-class fields](DATAMODEL.md#per-class-fields).

---

## `FlashMachine` — AnimationTree

`AnimationTree` subclass that drives a `FlashPlayer`. Pair with `AnimationNodeFlashClip` as the tree's root.

| Property | Type | Notes |
|----------|------|-------|
| `flash_player` | `NodePath` | Must point at a `FlashPlayer`. Hinted with `PROPERTY_HINT_NODE_PATH_VALID_TYPES` ([animation_node_flash.cpp:84](animation_node_flash.cpp#L84)). |
| `track` | `String` (enum) | `"[main]"` (drive `active_symbol`/`active_clip`) or one of the flash player's named tracks from `get_clips_tracks()` ([animation_node_flash.cpp:104-115](animation_node_flash.cpp#L104-L115)). |

`get_configuration_warning()` verifies the referenced node exists, is a `FlashPlayer`, and has a resource — emits scene-editor warnings otherwise.

---

## `AnimationNodeFlashClip` — AnimationRootNode

Root node of a `FlashMachine` tree. Picks one `(symbol, clip)` to drive.

| Parameter | Type | Notes |
|-----------|------|-------|
| `clip` | `String` | For `"[main]"` track: `"<symbol>"` or `"<symbol>/<clip>"` (slash-separated). For named tracks: the clip name only. The enum is populated dynamically from the player's symbols/clips ([animation_node_flash.cpp:167-191](animation_node_flash.cpp#L167-L191)). |
| `time` | `float` (tree parameter) | Auto-managed cursor. |

`process(p_time, p_seek)` returns the remaining time in the current clip — standard AnimationTree contract. For `"[main]"`, it splits the clip path, sets the player's symbol/clip, and calls `advance()`. For named tracks it routes through `advance_clip_for_track()`. See [animation_node_flash.cpp:206-246](animation_node_flash.cpp#L206-L246).

---

## Usage patterns

All snippets below are paraphrased from real game code in the private `math` game repo. File paths are given for context but are not linkable from here.

### 1. Simple playback + waiting for completion

Pattern: load a flash resource on demand, pick a symbol, play, wait for the built-in completion signal. Adapted from `math: game/assesments/core/genie.gd:585-601`.

```gdscript
onready var supplement_flash := $SupplementFlash  # a FlashPlayer

func give_five():
    var flash_resource = await lib.asyn.load("res://path/to/anim.zfl")
    if flash_resource:
        supplement_flash.resource = flash_resource
        supplement_flash.show()
        supplement_flash.set_active_timeline("tap_3_" + character)  # alias for active_symbol
        supplement_flash.playing = true
    yield(play("highfive_tap"), "completed")   # outer animation
    supplement_flash.playing = false
    supplement_flash.hide()
```

Note the use of `set_active_timeline` — the deprecated alias for `active_symbol`. New code should prefer `active_symbol`.

### 2. Multiple synchronized players with modulate

Pattern: pool several `FlashPlayer`s, recycle them round-robin, reset each via `set_frame(0)` + `modulate`, kick off with `playing = true`. From `math: game/egypt/scarab/logic/painter_socket.gd:81-107`.

```gdscript
var flash_arr: Array  # of FlashPlayer
var blocked_flash := {}
var next_flash_id := 0

func paint_anim(ball, time):
    var id = next_flash_id
    var flash: FlashPlayer = null
    if not blocked_flash.get(id, false):
        blocked_flash[id] = true
        flash = flash_arr[id]
        next_flash_id = (id + 1) % flash_arr.size()

        flash.modulate = place.get_paint_color()
        flash.set_frame(0)
        flash.show()
        flash.playing = true

    await Time.wait(time)
    if flash:
        flash.hide()
        blocked_flash[id] = false
```

### 3. Extending `FlashPlayer` with a clip queue

Pattern: subclass `FlashPlayer`, write a helper that sets `active_clip` and `yield`s on `animation_completed` before continuing. From `math: game/drills/rulers/logic/eyes.gd`.

```gdscript
extends FlashPlayer

func play_animation(clip: String) -> void:
    active_clip = clip
    yield(self, "animation_completed")

func play_blink_animation() -> void:
    var save := active_clip
    await play_animation("blink_1")
    await play_animation(save)
```

Relies on the deferred-emit behavior of `animation_completed` — the `yield`/`await` resumes after the current `advance()` returns, not mid-process.

### 4. Proxy player (sync another FlashPlayer's control signals)

Pattern: connect `set_active_clip_called`, `set_active_symbol_called`, `advance_called`, `advance_clip_for_track_called` on a source player to a wrapper that relays them (optionally with a frame offset). From `math: game/tutor/girl/hair.gd`.

This is why the "*_called" signals exist — they let one `FlashPlayer` follow another's playhead exactly, without polling.

---

## Gotchas

- **Vector shapes silently fail.** `FlashShape::parse()` returns `FAILED` at import; make sure your exporter rasterizes shapes.
- **`active_clip` is scoped to `active_symbol`.** Changing symbol resets clip to `""`. Always set symbol first.
- **Signals are deferred.** A handler that sets `active_clip` or calls `advance()` won't take effect until the next tick. Don't expect synchronous chaining.
- **Editor-mode suppression.** Inside the editor (`is_editor_hint`), `animation_completed` and `animation_event` are **not** emitted ([flash_player.cpp:971-976](flash_player.cpp#L971-L976), [1031-1037](flash_player.cpp#L1031-L1037)). Test playback behavior in-game, not on the editor preview.
- **Mask count cap.** Up to 4 mask items per masked element; clipping atlas is 32×32. Works for typical UI; heavy masking will hit the ceiling.
- **No per-node material.** `FlashPlayer` always draws with the internal shader. If you need custom blending, render into a `SubViewport` and apply material to the sprite that shows it.
- **No dynamic symbol creation.** `FlashDocument` is immutable after import. Author new content in Animate and re-import.
- **Deprecated aliases still seen in live code** — `set_active_label`/`set_active_timeline`. Mean the same as `active_clip`/`active_symbol`.
