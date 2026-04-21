# Flash module — data model & import pipeline

This document explains how a `.zfl` authored in Adobe Animate becomes a runtime-ready `FlashDocument` resource on disk, and what every field of that resource means. For runtime behavior see [HOWITWORKS.md](HOWITWORKS.md); for the GDScript API see [API.md](API.md).

## Import entry point

[`ResourceImporterFlash`](resource_importer_flash.h) registers with the editor via [`_editor_init()`](register_types.cpp#L89-L97).

| Property | Value | Source |
|----------|-------|--------|
| Importer name | `"flash"` | [resource_importer_flash.cpp:39-41](resource_importer_flash.cpp#L39-L41) |
| Visible name | `"Flash"` | [resource_importer_flash.cpp:43-45](resource_importer_flash.cpp#L43-L45) |
| Recognized extensions | `.zip`, `.zfl` | [resource_importer_flash.cpp:47-50](resource_importer_flash.cpp#L47-L50) |
| Save extension | `.res` | [resource_importer_flash.cpp:52-54](resource_importer_flash.cpp#L52-L54) |
| Resource type | `FlashDocument` | [resource_importer_flash.cpp:56-58](resource_importer_flash.cpp#L56-L58) |
| Importer version | `14` | [resource_importer_flash.cpp:37](resource_importer_flash.cpp#L37) |

Bumping `importer_version` invalidates all previously imported `FlashDocument`s and forces re-import when the editor next opens the project (`are_import_settings_valid()` checks this, [resource_importer_flash.cpp:75-116](resource_importer_flash.cpp#L75-L116)).

## Import options

Declared in [`get_import_options()`](resource_importer_flash.cpp#L64-L73):

| Option | Type | Default | Values |
|--------|------|---------|--------|
| `process/downscale` | enum | `Disabled` | `Disabled`, `x2`, `x4` — iterative `shrink_x2()` passes on every spritesheet. |
| `process/fix_alpha_border` | bool | `true` | Runs `Image::fix_alpha_edges()` after (re)sizing. |
| `compress/mode` | enum | `Video RAM (S3TC/ETC/BPTC)` | `Lossless (PNG)`, `Video RAM …`, `Uncompressed`. |
| `flags/repeat` | enum | `Disabled` | `Disabled`, `Enabled`, `Mirrored`. |
| `flags/filter` | bool | `true` | Linear filtering. |
| `flags/mipmaps` | bool | `true` | Also forced on when `compress/mode = VRAM`. |
| `flags/srgb` | enum | `Disable` | `Disable`, `Enable`. |

## What's inside a `.zfl`

The exporter ([Funexpected Flash Tools](https://github.com/funexpected/flash-tools)) produces a zip containing, at minimum:

```
<something>/
  DOMDocument.xml        # timeline/symbol/frame/element tree (one file total)
  spritesheets.list      # newline-separated list of spritesheet basenames
  sheet_0.png            # one or more spritesheet images
  sheet_0.json           # TexturePacker-style atlas manifest
  sheet_1.png
  sheet_1.json
  ...
```

`DOMDocument.xml` is the same format Adobe Animate uses for its internal project files (XFL), parsed element by element via Godot's `XMLParser`. The `frames` dictionary inside each `sheet_N.json` maps a bitmap name (e.g. `gdexp/character/head_0`) to an atlas region.

## Import steps

[`ResourceImporterFlash::import()`](resource_importer_flash.cpp#L118-L423):

1. **Unzip** the `.zfl` into `<p_save_path>.tmp/` using `minizip` + Godot's `zipio`. Locates `DOMDocument.xml` along the way ([resource_importer_flash.cpp:141-181](resource_importer_flash.cpp#L141-L181)).
2. **Parse XML** via [`FlashDocument::from_file(document_path)`](flash_resources.cpp#L249-L253), producing a populated `FlashDocument` with timelines, symbols, bitmaps, and the variants dictionary — but no atlas yet.
3. **Load spritesheets**: read `spritesheets.list`, then for each sheet parse its JSON manifest (`frames` dict) and load the PNG ([resource_importer_flash.cpp:234-274](resource_importer_flash.cpp#L234-L274)). The importer tags each frame's dict with `texture_idx = <sheet-index>` so the later atlas lookup knows which TextureArray layer to use.
4. **Apply downscale / alpha-fix** — each spritesheet is optionally shrunk via repeated `shrink_x2()` plus `fix_alpha_edges()` ([resource_importer_flash.cpp:263-271](resource_importer_flash.cpp#L263-L271)).
5. **Hook bitmaps to atlas regions** — for every `FlashBitmapItem` in the document, look up the matching frame by name (`name.replace_first("gdexp/", "")`), build a `FlashTextureRect { index, region, original_size }`, and assign it to `item.texture` ([resource_importer_flash.cpp:276-301](resource_importer_flash.cpp#L276-L301)).
6. **Save texture(s)** — [`_save_tex()`](resource_importer_flash.cpp#L425-L490) serializes the spritesheet images + metadata into a FastLZ-compressed blob written to a `.ftex` file. In VRAM mode, one `.ftex` per selected compression (`s3tc`, `etc2`, `etc`, `pvrtc`), plus a dummy `Server` variant with empty images ([resource_importer_flash.cpp:305-356](resource_importer_flash.cpp#L305-L356)).
7. **Save the document** — `ResourceSaver::save()` writes the populated `FlashDocument` as `<save_path>[.variant].res`. Its `atlas` field holds a `Ref<TextureArray>` loaded back from the corresponding `.ftex` ([resource_importer_flash.cpp:315, 326, 335, 344, 352, 365](resource_importer_flash.cpp#L315)).

## On-disk artifacts

For a source `my_anim.zfl`, after import you will see (paths are approximate — Godot places imported files under `.godot/imported/`):

| File | Content |
|------|---------|
| `my_anim.zfl.import` | Godot's ConfigFile describing how to import this resource. |
| `my_anim-<hash>.res` (or `.s3tc.res`, `.etc2.res`, `.etc.res`, `.pvrtc.res`, `.Server.res` in VRAM mode) | Binary-serialized `FlashDocument`. |
| `my_anim-<hash>.ftex` (or per-variant `.s3tc.ftex`, etc.) | Custom FastLZ-compressed texture blob. Loaded by [`ResourceFormatLoaderFlashTexture`](flash_resources.h#L510-L516) into a `TextureArray`. |

The `.ftex` layout, read by `ResourceFormatLoaderFlashTexture::load()` (see [flash_resources.cpp](flash_resources.cpp) for the inverse of `_save_tex()`):

- `uint32 uncompressed_size`
- FastLZ-compressed variant-encoded `Dictionary` with keys:
  - `width`, `height`, `flags`, `format` (`Image::Format`)
  - `images: Array<Image>` — one layer per spritesheet

At project export time, [`EditorExportFlash`](register_types.cpp#L39-L87) reads the `.import` ConfigFile, picks the `.res` variant matching the export target's `features`, and pulls the sibling `.ftex` into the PCK.

## Class hierarchy

All resource types derive from `FlashElement`, a `Resource` subclass that tracks its owning document and parent. Virtual classes are italicized.

```
Resource
├── FlashElement (virtual)            flash_resources.h:69
│   ├── FlashDocument                 flash_resources.h:128
│   ├── FlashBitmapItem               flash_resources.h:186
│   ├── FlashTimeline                 flash_resources.h:208
│   ├── FlashLayer                    flash_resources.h:258
│   ├── FlashFrame                    flash_resources.h:315
│   ├── FlashDrawing (virtual)        flash_resources.h:301
│   │   ├── FlashInstance             flash_resources.h:368
│   │   ├── FlashBitmapInstance       flash_resources.h:436
│   │   ├── FlashShape                flash_resources.h:411  (stub — not rendered)
│   │   └── FlashGroup                flash_resources.h:419
│   └── FlashTween                    flash_resources.h:461
└── FlashTextureRect                  flash_resources.h:101  (not a FlashElement)
```

`FlashElement` carries `document`, `parent`, `eid` (internal element id) — see [flash_resources.h:72-99](flash_resources.h#L72-L99).

## Per-class fields

### `FlashDocument` — [flash_resources.h:128-184](flash_resources.h#L128-L184)

| Field | Type | Role |
|-------|------|------|
| `document_path` | `String` | Temp directory the XML was parsed from (used during import to resolve relative spritesheet paths). |
| `symbols` | `Dictionary` | All named timelines, keyed by validated token (symbol name). Populated from `<DOMSymbolItem>` parsing. |
| `bitmaps` | `Dictionary` | All `FlashBitmapItem`s, keyed by bitmap name. |
| `frame_size` | `float` | `1.0 / 24` by default — frame period used when `FlashTimeline` doesn't override. |
| `timelines` | `List<Ref<FlashTimeline>>` | Document-level timelines (the first one is the main timeline). |
| `atlas` | `Ref<TextureArray>` | Packed spritesheets (one layer per sheet). Assigned by the importer after `_save_tex()`. |
| `variants` | `Dictionary` | Nested — `variants[layer_name][variant_name][symbol_token] = frame_index`. Populated by `cache_variants()` from `anchor` labels. |
| `variated_symbols_count` | `int` | Count of symbols that participate in at least one variant (used for enum sizing in the editor). |
| `last_eid` | `int` | Monotonic element-id counter used during parse to give every `FlashElement` a unique id. |

### `FlashTimeline` — [flash_resources.h:208-256](flash_resources.h#L208-L256)

| Field | Type | Role |
|-------|------|------|
| `token` | `String` | Symbol name (key under `FlashDocument.symbols`). |
| `local_path` | `String` | Original path inside the zip. |
| `clips_header` | `String` | Optional grouping label emitted by the exporter for UI grouping. |
| `duration` | `int` | Total frame count of the timeline. |
| `clips` | `Dictionary` | `name → Vector2(start, end)` — named frame ranges (see `name` label type in HOWITWORKS.md). |
| `events` | `Dictionary` | `name → PoolRealArray` of frame indices where this event fires (from `comment` labels). |
| `variants` | `Dictionary` | Subset of the document's variants dict relevant to this timeline. |
| `layers` | `List<Ref<FlashLayer>>` | Ordered list of normal layers. |
| `masks` | `List<Ref<FlashLayer>>` | Layers declared with `type = "mask"`. |
| `variation_idx` | `int` | Index of the variation frame for this timeline (used with anchor variants); `-1` if none. |

### `FlashLayer` — [flash_resources.h:258-299](flash_resources.h#L258-L299)

| Field | Type | Role |
|-------|------|------|
| `index` | `int` | Z-order within the timeline (higher = drawn later). |
| `layer_name` | `String` | Author-set name. Used as the "track" identifier for `FlashMachine` and for variant keys. |
| `type` | `String` | `""`, `"mask"`, `"masked"`, `"guide"` (guide is exporter-stripped). |
| `duration` | `int` | Convenience — length in frames. |
| `mask_id` | `int` | When `type = "masked"`, points at the `eid` of the mask layer above it. |
| `color` | `Color` | Author-UI color (not used at runtime). |
| `frames` | `List<Ref<FlashFrame>>` | Keyframes in order. |

### `FlashFrame` — [flash_resources.h:315-366](flash_resources.h#L315-L366)

| Field | Type | Role |
|-------|------|------|
| `index` | `int` | First frame index this keyframe covers. |
| `duration` | `int` | How many frames this keyframe lasts before the next one. |
| `frame_name` | `String` | Label text (if `label_type` is set). |
| `label_type` | `String` | `""`, `"name"` (defines a clip), `"anchor"` (variant marker), `"comment"` (animation event). |
| `keymode` | `String` | Mirrors Animate's keyframe mode; mostly informational. |
| `tween_type` | `String` | `"none"` or a hint used during parsing. |
| `color_effect` | `FlashColorEffect` | Per-frame color add/mult. |
| `elements` | `List<Ref<FlashDrawing>>` | The drawing children rendered on this frame. |
| `tweens` | `List<Ref<FlashTween>>` | Easings applied while this frame is the active keyframe. |

### `FlashInstance` — [flash_resources.h:368-409](flash_resources.h#L368-L409)

| Field | Type | Role |
|-------|------|------|
| `transform` | `Transform2D` | Local transform applied to the nested timeline. |
| `center_point`, `transformation_point` | `Vector2` | Mirror the Animate pivots — used when the exporter re-anchors rotations/scales. |
| `first_frame` | `int` | Which frame of the nested timeline to start at (used when `loop = "single frame"`). |
| `loop` | `String` | `"loop"`, `"play once"`, `"single frame"` ([flash_resources.cpp:961](flash_resources.cpp#L961)). |
| `timeline_token` | `String` | Symbol name → looked up in `FlashDocument.symbols`. |
| `layer_name` | `String` | Name of the layer this instance sits on in the parent timeline (used for variant lookups). |
| `timeline` | `FlashTimeline*` | Resolved cache pointer (raw, not `Ref`) to avoid per-frame dictionary lookups. |
| `color_effect` | `FlashColorEffect` | Per-instance color add/mult, composed with frame effects. |

### `FlashBitmapInstance` — [flash_resources.h:436-459](flash_resources.h#L436-L459)

| Field | Type | Role |
|-------|------|------|
| `transform` | `Transform2D` | Inherited from `FlashDrawing`. |
| `library_item_name` | `String` | Bitmap library name. Looked up in `FlashDocument.bitmaps`, whose entry carries a `FlashTextureRect`. |
| `texture` | `Ref<FlashTextureRect>` | Cached pointer to the atlas rect for this bitmap. |
| `uvs` | `Vector<Vector2>` | Cached UV coords (built once per instance the first time it renders). |

### `FlashGroup` — [flash_resources.h:419-433](flash_resources.h#L419-L433)

Just a `List<Ref<FlashDrawing>> members` plus a transform. Groups decompose recursively at render time.

### `FlashTween` — [flash_resources.h:461-506](flash_resources.h#L461-L506)

| Field | Type | Role |
|-------|------|------|
| `target` | `String` | `"all"` (default) or a single property name. Current implementation animates all properties together; per-property tweening is declared in README but not done. |
| `method` | `Method` enum | `NONE`, `CLASSIC`, the standard easing families, or `CUSTOM`. |
| `intensity` | `float` | `CLASSIC` tween intensity (mirrors Animate's -100..100 slider). |
| `points` | `PoolVector2Array` | Control-curve points used by `CUSTOM` method. `interpolate(time)` walks them. |

### `FlashTextureRect` — [flash_resources.h:101-126](flash_resources.h#L101-L126)

Not a `FlashElement` — a small helper resource describing a rectangle in the document's atlas.

| Field | Type | Role |
|-------|------|------|
| `index` | `int` | Layer index in the `TextureArray` (which spritesheet). |
| `region` | `Rect2` | Pixel rect within that layer. |
| `margin` | `Rect2` | Trim/padding hint (reserved; unused at runtime today). |
| `original_size` | `Vector2` | Size before any `downscale` passes — kept so runtime can know the authored dimensions. |

## Symbol references

Nested symbols reference each other **by string token**, not by `Ref<>`. A `FlashInstance.timeline_token` is looked up in `FlashDocument.symbols` (a `Dictionary` keyed on the symbol's validated name, [flash_resources.cpp:271-274](flash_resources.cpp#L271-L274)). This keeps the resource tree a DAG even when Animate allows cyclic nesting at authoring time.

The resolved `FlashTimeline*` is cached on the instance at setup time (not serialized) to avoid per-frame lookups during playback.

## Atlas layout

- **One `TextureArray` per document**, stored on `FlashDocument.atlas`.
- **One array layer per spritesheet PNG** from the `.zfl`.
- **Each bitmap is a `FlashTextureRect { index, region, original_size }`** — `index` selects the array layer, `region` is the pixel rect within it.
- Atlas size / layer count is fixed at import time; the runtime never reshapes it.

## Variants (skinning)

At import, frames labeled `"anchor"` contribute to `FlashTimeline.variants` and roll up into `FlashDocument.variants` via `cache_variants()`. The runtime selects between them with `FlashPlayer.set_variant(layer_name, variant_name)` — the `active_variants: HashMap<String, String>` on the player drives which frame a nested symbol shows on the tagged layer.

Use case: one rig with multiple outfits. The layer carrying the outfit symbol has anchor labels `"red"`, `"blue"`, `"gold"`, and `set_variant("outfit_layer", "gold")` picks the right pose per player.

## Serialization notes & quirks

- **Shapes never serialize meaningfully** — `FlashShape::parse()` returns `FAILED` by design; the element exists in the class hierarchy but is an authoring-time stub.
- **Tweens are stored per-frame**, not per-property. Applied during `animation_process()` by interpolating the frame's properties as a block.
- **Clipping data is runtime-only** — never serialized. `FlashPlayer` rebuilds the 32×32 clipping image every frame from the current mask stack.
- **UV caching is one-shot** — `FlashBitmapInstance.uvs` is filled on the first render and assumed stable thereafter. Mutating the atlas at runtime would break this.
- **Server variant** — the `.Server.ftex` contains no image data ([resource_importer_flash.cpp:349-355](resource_importer_flash.cpp#L349-L355)) but preserves the resource chain so headless builds can still load documents without paying texture memory.

## Reimport triggers

Anything that changes `importer_version` ([resource_importer_flash.cpp:37](resource_importer_flash.cpp#L37)) invalidates every imported `FlashDocument`. Per-asset reimport also happens when the `.zfl` modification time changes, or when required VRAM formats (`rendering/vram_compression/import_*` in ProjectSettings) are added since last import — see [`are_import_settings_valid()`](resource_importer_flash.cpp#L75-L116).
