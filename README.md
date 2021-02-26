## About
This module adds [Adobe Animate](https://www.adobe.com/products/animate.html) animation support for Godot Engine. `FlashPlayer` node available to draw a flash document (imported as` FlashDocument` resource) within a single draw call. You can use the precompiled editor (see [releases](https://github.com/funexpected/godot-flash-module/releases)) or build editor and templates by yourself.

## Compilation
Add this repo to `modules/flash` folder of Godot Engine source. See engine [compilation instructions](https://docs.godotengine.org/en/stable/development/compiling/index.html) instructions.
`` bash
cd /path/to/godot
git submodule add git@github.com:funexpected/godot-flash-module.git modules/flash
scons -j8
``

## Usage
- Install [Funexpected Flash Tools](https://github.com/funexpected/flash-tools) plugin.
- Export your `Amazing Project.xfl` Flash project using `Commands/Funepxected Tools/Export` to` amazing_project.zfl`
- Add `amazing_project.zfl` to Godot project
- Add `FlashPlayer` node to the scene
- Set `Resource` property of` FlashPlayer` to `res: // amazing_project.zfl`
- You can switch active timeline, label (if it present), and change skins of symbols.

## Supported and planed features
- [x] Bitmaps (full rasterization prepared by [Funexpected Flash Tools](https://github.com/funexpected/flash-tools))
- [x] Groups (decomposed and rasterized)
- [ ] Shapes
- [x] Name Labels (for splitting single animation to multiply parts)
- [x] Anchor Labels (for skinning, e.g. forcing custom frames in target symbols)
- [ ] Comment Labels (for user-defined signals)
- [ ] Predefined signals (e.g. `animation_completed`,` timeline_changed`)
- [x] Looping (loop, once, single frame)
- [x] Color effects
- [x] Tweening (for properties together)
- [ ] Tweening (for properties separately)
- [x] Masks (up to 4 masking elements per masked element)
- [ ] Motion guides
- [x] Mutltiply spritesheets packed by [Funexpected Flash Tools](https://github.com/funexpected/flash-tools)
- [ ] Warp tool
- [ ] Sounds
- [ ] Custom material (for now, you can't change material for `FlashPlayer` node)
- [ ] Blending (not in near future)
- [ ] Filters (not in near future)
- [x] Downscaling spritesheets on import time
- [x] Custom properties for importing textures (loseless/vram/ uncompressed, mipmaps, filter)
- [x] Compressing VRAM textures (reducing disk space of exported Godot project)