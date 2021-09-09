// MIT License

// Copyright (c) 2021 Yakov Borevich, Funexpected LLC

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifdef MODULE_FLASH_ENABLED
#ifndef FLASH_PLAYER_H
#define FLASH_PLAYER_H

#include <scene/2d/node_2d.h>

#include "flash_resources.h"

class FlashDocument;
class FlashTimeline;

struct FlashMaskItem {
    Transform2D transform;
    Rect2 texture_region;
    int texture_idx;
};

class FlashPlayer: public Node2D {
    GDCLASS(FlashPlayer, Node2D);

    // renderer part
    float frame;
    float frame_rate;
    bool playing;
    float playback_start;
    float playback_end;
    bool tracks_dirty;
    float queued_delta;
    bool animation_process_queued;
    Ref<FlashDocument> resource;
    String active_symbol_name;
    Ref<FlashTimeline> active_symbol;
    String active_clip;
    bool loop;
    RID flash_material;
    RID mesh;
    static RID flash_shader;

    // batcher part
    float processed_frame;
    int cliping_depth;
    Vector<Vector2> points;
    Vector<Vector2> uvs;
    Vector<Color> colors;
    Vector<int> indices;
    List<String> events;

    HashMap<String, Vector3> clips_state;
    HashMap<String, String> active_clips;
    Ref<Image> clipping_data;
    Ref<ImageTexture> clipping_texture;
    HashMap<int, List<FlashMaskItem>> masks;
    List<int> mask_stack;
    Vector<int> frame_overrides;
    HashMap<String, String> active_variants;
    List<FlashMaskItem> clipping_cache;
    List<FlashMaskItem> clipping_items;
    int current_mask;


    int performance_triangles_drawn;
	int performance_triangles_generated;

protected:
    void _notification(int p_what);
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
    virtual void _validate_property(PropertyInfo &prop) const;
	static void _bind_methods();
    bool _sort_clips(Variant a, Variant b) const;

public:
    FlashPlayer();
    ~FlashPlayer();

    float get_frame() const { return frame; }
    void set_frame(float p_frame) { frame = p_frame; update(); }
    void override_frame(String p_symbol, Variant p_frame);
    void set_variant(String key, Variant value);
    String get_variant(String key) const;
    void set_clip(String header, Variant value);
    String get_clip(String header) const;
    PoolStringArray get_clips_tracks() const;
    PoolStringArray get_clips_for_track(const String &track) const;
    float get_clip_duration(const String &track, const String &clip) const;
    Dictionary get_variants() const;
    float get_symbol_frame(FlashTimeline* symbol, float p_default);
    float get_frame_rate() const { return frame_rate; }
    void set_frame_rate(float p_frame_rate) { frame_rate = p_frame_rate; }
    bool is_playing() const { return playing; }
    void set_playing(bool p_playing) { playing = p_playing; }
    bool is_loop() const { return loop; }
    void set_loop(bool p_loop) { loop = p_loop; }
    Ref<FlashDocument> get_resource() const;
    void set_resource(const Ref<FlashDocument> &doc);
    float get_duration(String symbol=String(), String label=String());
    String get_active_symbol() const;
    void set_active_symbol(String p_symbol);
    String get_active_clip() const;
    void set_active_clip(String p_clip);
    PoolStringArray get_symbols() const;
    PoolStringArray get_clips(String p_symbol=String()) const;

    // batcher part
    void queue_animation_process();
    void queue_process(float delta=0.0);
    void _animation_process();
    void advance(float p_delta, bool p_skip=false, bool advance_all_tracks=false);
    void advance_clip_for_track(const String &p_track, const String &p_clip, float delta=0.0, bool p_skip=false, float *r_elapsed=NULL, float *r_ramaining=NULL);
    void update_clipping_data();
    void add_polygon(Vector<Vector2> p_points, Vector<Color> p_colors, Vector<Vector2> p_uvs, int p_texture_idx);
    void queue_animation_event(const String &p_name, bool p_reversed=false);

    bool is_masking();
    void mask_begin(int layer);
    void mask_add(Transform2D p_transform, Rect2i p_texture_region, int p_texture_idx);
    void mask_end(int layer);

    void clip_begin(int layer);
    void clip_end(int layer);
};


#endif
#endif