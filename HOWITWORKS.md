# Flash module — how it works (runtime)

This document covers what happens at **runtime**: how a loaded `FlashDocument` is played by a `FlashPlayer`, how it renders, and how it integrates with Godot's AnimationTree. For import and on-disk layout, see [DATAMODEL.md](DATAMODEL.md); for the GDScript surface, see [API.md](API.md).

## Pipeline overview

```
Animate (.fla)
    │
    │  Funexpected Flash Tools (export command)
    ▼
.zfl   (zip: DOMDocument.xml + spritesheets.png/.json + spritesheets.list)
    │
    │  ResourceImporterFlash  (editor-only)
    ▼
.res   (serialized FlashDocument)
+ .ftex (TextureArray — possibly multiple per-platform variants)
    │
    │  Runtime: ResourceLoader
    ▼
FlashPlayer (Node2D) → renders via VisualServer custom shader
```

See [DATAMODEL.md](DATAMODEL.md) for the editor-side half. This document starts at the runtime boundary.

## Registration

[`register_flash_types()`](register_types.cpp#L103-L132) registers the scene node, the AnimationTree classes, and all `FlashElement` Resource subclasses via `ClassDB`. In editor builds, [`_editor_init()`](register_types.cpp#L89-L97) additionally installs:

- `ResourceImporterFlash` as a `ResourceFormatImporter` (imports `.zfl`/`.zip`).
- `EditorExportFlash` as an `EditorExportPlugin` (packs the right `.ftex` variant per platform during project export — [register_types.cpp:39-87](register_types.cpp#L39-L87)).

A `ResourceFormatLoaderFlashTexture` is registered in all builds so `.ftex` files can be loaded at runtime ([register_types.cpp:126-127](register_types.cpp#L126-L127)).

## Scene integration

`FlashPlayer` is a `Node2D` ([flash_player.h:42](flash_player.h#L42)). It owns a `Ref<FlashDocument>` as its `resource` property plus two string pointers into that document:

- `active_symbol_name` → a key in `FlashDocument.symbols`, resolved to `active_symbol: Ref<FlashTimeline>`.
- `active_clip` → a named label *inside* that timeline, giving a start/end frame range.

If `active_symbol_name` is empty, the document's main timeline is used (`resource->get_main_timeline()`, resolved in [flash_player.cpp:818-839](flash_player.cpp#L818-L839)).

Playback state: `frame` (float), `frame_rate` (default 24), `playing`, `loop`, `playback_start`, `playback_end`.

## Playback loop

Per-frame flow in [flash_player.cpp:35-64](flash_player.cpp#L35-L64):

1. `NOTIFICATION_READY` enables `set_process(true)`.
2. `NOTIFICATION_PROCESS` calls `advance(get_process_delta_time(), false, true)` when `playing && active_symbol.is_valid()`.
3. `NOTIFICATION_DRAW` dispatches to either `_draw_normal()` or `_draw_metaball()` depending on `render_mode`.

`advance(time, seek, advance_all_tracks)` ([flash_player.cpp:983-1040](flash_player.cpp#L983-L1040)):

- Emits `advance_called` signal.
- Converts seconds → frames via `frame_rate`.
- If `seek`: `frame = playback_start + delta`; otherwise `frame += delta`.
- If `advance_all_tracks`: advances every entry in `clips_state` (the per-track clip cursors) by `delta`.
- Handles end-of-clip: loops back or clamps and emits `animation_completed` (deferred, never inside process to avoid recursion).
- Calls `queue_process(delta)` which eventually runs `_animation_process()`.

`_animation_process()` ([flash_player.cpp:944-981](flash_player.cpp#L944-L981)):

- Clears the accumulated `indices`, `points`, `colors`, `uvs` buffers.
- Invokes `active_symbol->animation_process(this, frame, queued_delta)`, which walks the timeline tree and appends primitives to the player's buffers.
- Calls `update()` to schedule a redraw.
- Emits any queued `animation_event(name)` signals (deferred).

### Timeline cascade

The `animation_process()` virtual on each element walks down the document:

| Class | Behavior |
|-------|----------|
| `FlashTimeline` | Iterates `layers` **back-to-front** (so higher layer index draws last / on top); each layer sees the same `(time, delta)`. Handles masks and clip begin/end. |
| `FlashLayer` | Finds the active `FlashFrame` for the current frame index and delegates. |
| `FlashFrame` | Applies frame-local `tweens` to interpolate properties, then calls `animation_process` on every `FlashDrawing` element. |
| `FlashInstance` | Resolves `timeline_token` → nested `FlashTimeline`. Handles `loop` mode (`"loop"` / `"play once"` / `"single frame"`, see [flash_resources.cpp:961](flash_resources.cpp#L961)) and recurses. |
| `FlashBitmapInstance` | Pushes a quad into the player's batch via `add_polygon(...)` ([flash_player.cpp:185-188](flash_player.h#L185-L188)) with UVs from the atlas and the composed transform + color effect. |
| `FlashGroup` | Recurses into its `members`. |
| `FlashShape` | Not supported — `parse()` returns `FAILED` at import; no runtime branch exists. |

Transforms compose parent→child: each step applies its own `Transform2D` to the accumulated `tr` and passes the result down. Color effects compose via `FlashColorEffect::operator*` ([flash_resources.h:57-62](flash_resources.h#L57-L62)).

## Rendering

Custom GLSL shader embedded in `_generate_flash_shader()` ([flash_player.cpp:66-200](flash_player.cpp#L66-L200)):

- `shader_type canvas_item`.
- `uniform sampler2DArray ATLAS` — the document's texture atlas layers.
- `uniform sampler2D CLIPPING_TEXTURE` — a tiny (32×32) dynamic texture encoding up-to-4 mask rectangles.
- `varying float CLIPPING_IDX[4]`, `CLIPPING_UV[4]` — per-vertex mask parameters.
- `uniform bool OVERLAY_ENABLED` / `OVERLAY_TEXTURE` — optional per-player replacement texture (`replace_texture` property).
- Metaball uniforms: `CIRCLE_0…CIRCLE_15`, `THRESHOLD`, `CIRCLES_COUNT`, `METABALL_ENABLED`, `DEBUG_ENABLED`.

Every `FlashPlayer` builds a single `VisualServer` mesh each frame from the accumulated `indices/points/uvs/colors` buffers — one draw call per player regardless of how many bitmaps, symbols, or masks it contains.

### Render modes

| Mode | What it does |
|------|--------------|
| `RENDER_NORMAL` | Straightforward atlas blit + mask/clip + optional overlay. `_draw_normal()`. |
| `RENDER_METABALL` | Experimental. Each bitmap instance submits an `add_metaball(pos, radius)` circle ([flash_player.h:186](flash_player.h#L186)); shader unions them via a distance-based sum with a `THRESHOLD` cutoff. `metaball/color`, `metaball/balancing`, `metaball/threshold`, `metaball/debug` properties control the effect. Intended for blobby particle effects. |

## Transforms and color effects

- `Transform2D` is Godot's standard 2D affine matrix. Composition is parent-multiplied-by-child (see the `tr` parameter threaded through every `animation_process()`).
- `FlashColorEffect` ([flash_resources.h:42-67](flash_resources.h#L42-L67)) stores `add: Color` + `mult: Color`. Combines: `new.mult = a.mult * b.mult`, `new.add = a.add * b.mult + b.add`. Interpolated linearly across a tween via `interpolate(target, amount)`.

## Masking

Implemented in `FlashPlayer::mask_begin/mask_add/mask_end` and `clip_begin/clip_end` ([flash_player.h:189-195](flash_player.h#L189-L195)). Layers with `type = "mask"` set `mask_id` on subsequent layers; when the masker is drawn the mask items are appended to the mask stack for the masked layers.

- Up to **4 mask items per masked element** (enforced by the shader's `CLIPPING_UV[4]` array).
- Each frame, accumulated mask transforms + atlas regions are packed into a 32×32 `ImageTexture` (`clipping_texture`) via `update_clipping_data()` — see [flash_player.h:93-101](flash_player.h#L93-L101) for the fields. The shader samples this texture to reject fragments outside the masks.
- Cost: the 32×32 image is rebuilt and re-uploaded every frame. Fine for a handful of masked regions, worth knowing if you add many masked layers.

## Tweens

`FlashTween` ([flash_resources.h:461-506](flash_resources.h#L461-L506)):

- `method`: enum with the Flash easing families — `CLASSIC`, `IN_QUAD/OUT_QUAD/INOUT_QUAD`, `IN_CUBIC/OUT_CUBIC/INOUT_CUBIC`, …, `IN_ELASTIC/OUT_ELASTIC/INOUT_ELASTIC`, or `CUSTOM`.
- `CUSTOM` uses `points: PoolVector2Array` as a Bezier-style control curve.
- `target = "all"` means all properties interpolate together (current limitation — see [README.md:31](README.md#L31)); a non-`"all"` target would scope the tween to one property but this path is incomplete.
- `interpolate(time) → float` returns the eased 0..1 factor for frame interpolation.

Tweens live on a `FlashFrame` and apply while that frame is the active frame in its layer ([flash_resources.h:326-329](flash_resources.h#L326-L329)).

## Events and labels

A `FlashFrame` carries both a `frame_name` and a `label_type` ([flash_resources.h:320-324](flash_resources.h#L320-L324)):

| `label_type` | Meaning |
|--------------|---------|
| `"name"` | Starts a *clip* — a named sub-range of frames in the enclosing timeline. Registered in `FlashTimeline.clips: Dictionary` as `name → Vector2(start, end)`. Selectable at runtime via `FlashPlayer.active_clip`. |
| `"anchor"` | Marks a skinning variant frame (see *Variants* below). |
| `"comment"` | Triggers an `animation_event(name)` signal when the frame is crossed. Accumulated in `FlashPlayer.events` and emitted deferred. |

`animation_completed` is always emitted at the end of a clip (or the end of the symbol if no clip is active), via `call_deferred` to avoid recursion ([flash_player.cpp:1031-1037](flash_player.cpp#L1031-L1037)).

## Variants (skinning)

`FlashDocument.variants` is a nested dictionary — `variants[layer_name][variant_name][symbol_token] = frame_index`. Setting `FlashPlayer.set_variant(layer_name, variant_name)` stores the selection in `active_variants`; during playback this overrides the frame index used when rendering instances on that layer. Use case: one character-rig timeline with multiple costumes, selected per-player at runtime.

Anchors are populated at import from frames whose label type is `"anchor"`.

## AnimationTree integration

Two classes extend Godot's AnimationTree system ([animation_node_flash.h](animation_node_flash.h)):

- **`FlashMachine`** (`AnimationTree` subclass) — points at a `FlashPlayer` via `flash_player: NodePath` and picks a `track: String` to drive. Tracks are the named layers a `FlashPlayer` exposes via `get_clips_tracks()`; the special value `"[main]"` drives the `active_symbol`/`active_clip` pair directly ([animation_node_flash.cpp:104-115](animation_node_flash.cpp#L104-L115)).
- **`AnimationNodeFlashClip`** (`AnimationRootNode`) — root node of the tree. Exposes a `clip: String` parameter and a `time` parameter (auto-advanced). On `process(p_time, p_seek)` it either (a) for `"[main]"` tracks: splits `clip` on `/` into `(symbol, clip)`, calls `set_active_symbol/set_active_clip` and `advance()`, or (b) for a named track: calls `advance_clip_for_track(track, clip, step, seek, &elapsed, &remaining)`. See [animation_node_flash.cpp:206-246](animation_node_flash.cpp#L206-L246).

This lets a state-machine AnimationTree blend between clips on the same `FlashPlayer`, with per-track cursors stored in the player's `clips_state` map. The `advance_clip_for_track_wrapper` method wraps the output-parameter version so it's scriptable ([flash_player.cpp:1042-1045](flash_player.cpp#L1042-L1045)).

## Known limitations

From [README.md](README.md) (feature checklist):

- **No vector shapes** — the exporter must rasterize.
- **No sound** — no audio handling path exists in the module.
- **No blending modes** — Godot's 2D CanvasItem blend mode is the only option (set via modulate).
- **No filters** — Flash filter effects (blur, glow, …) not implemented.
- **No motion guides** — only linear or eased tweens, no path-following.
- **No runtime material override** — the player always uses the internal `flash_shader`.
- **No dynamic symbol creation** — `FlashDocument` is immutable after import.

### Implementation-level gotchas

- Mask stack limited to 4 items per masked element.
- Clipping-data image is 32×32 — ample for typical use but a hard ceiling if many masks are stacked.
- `animation_completed` and `animation_event` are always deferred — a handler setting `active_clip` will take effect the next tick, not the current one.
- In editor (`TOOLS_ENABLED` and `is_editor_hint()`), event and completion signals are suppressed to avoid running gameplay logic in-editor ([flash_player.cpp:971-976](flash_player.cpp#L971-L976)).
