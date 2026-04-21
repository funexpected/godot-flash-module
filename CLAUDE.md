# modules/flash — Adobe Animate playback

Plays Adobe Animate projects inside Godot 3.3 as a `Node2D`. Authoring happens in Animate + the [Funexpected Flash Tools](https://github.com/funexpected/flash-tools) plugin, which exports a `.zfl` archive; this module imports the archive and renders it at runtime with full timeline, nested-symbol, and masking support, in a single batched draw call per player.

**Playback only.** No native `.swf` parsing — the exporter produces JSON + pre-rasterized PNG spritesheets.

## Key constraints

- Vector shapes are not drawn at runtime — the exporter must rasterize them (`FlashShape::parse()` returns `FAILED` by design, see [HOWITWORKS.md](HOWITWORKS.md#known-limitations)).
- No sound, no blending, no filters, no motion guides (see [README.md](README.md) feature checklist).
- Up to 4 mask items per masked element.
- `FlashPlayer` has no per-node material override.

## Deep-dive docs

| Doc | Covers |
|-----|--------|
| [HOWITWORKS.md](HOWITWORKS.md) | Runtime pipeline — playback loop, rendering, masking, tweens, AnimationTree integration. |
| [DATAMODEL.md](DATAMODEL.md) | Import pipeline and on-disk resource layout. How `.zfl` becomes `.res` + `.ftex`. Class hierarchy of `FlashDocument` and friends. |
| [API.md](API.md) | GDScript API reference with glossary, per-class tables, and usage examples lifted from the private `math` game repo. |

## Source files

| File | Role |
|------|------|
| [flash_player.h](flash_player.h) / [flash_player.cpp](flash_player.cpp) | `FlashPlayer` Node2D — playback, rendering, shader, batching. |
| [flash_resources.h](flash_resources.h) / [flash_resources.cpp](flash_resources.cpp) | `FlashDocument` + all element Resource classes. XML parser. |
| [resource_importer_flash.h](resource_importer_flash.h) / [.cpp](resource_importer_flash.cpp) | `ResourceImporterFlash` — imports `.zfl`/`.zip` to `.res` + `.ftex`. |
| [animation_node_flash.h](animation_node_flash.h) / [.cpp](animation_node_flash.cpp) | `FlashMachine` (AnimationTree) + `AnimationNodeFlashClip`. |
| [register_types.cpp](register_types.cpp) | ClassDB registration, editor-only importer/exporter registration. |

## Related

- [README.md](README.md) — public-facing feature checklist.
- Upstream (open-source mirror): https://github.com/funexpected/godot-flash-module
- Authoring tool: https://github.com/funexpected/flash-tools
- Consumers: 60+ `.zfl` assets in the private `math` game repo.
