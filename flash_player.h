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
};

class FlashPlayer: public Node2D {
    GDCLASS(FlashPlayer, Node2D);

    // renderer part
    float frame;
    float frame_rate;
    bool playing;
    float playback_start;
    float playback_end;
    Ref<FlashDocument> resource;
    String active_timeline_name;
    Ref<FlashTimeline> active_timeline;
    String active_label;
    bool loop;
    RID flash_material;
    RID flash_shader;

    // batcher part
    float batched_frame;
    int cliping_depth;
    Vector<Vector2> points;
    Vector<Vector2> uvs;
    Vector<Color> colors;
    Vector<int> indices;
    
    Ref<Image> clipping_data;
    Ref<ImageTexture> clipping_texture;
    HashMap<int, List<FlashMaskItem>> masks;
    HashMap<String, int> frame_overrides;
    HashMap<String, String> active_variants;
    List<FlashMaskItem> clipping_cache;
    List<FlashMaskItem> clipping_items;
    int current_mask;


protected:
    void _notification(int p_what);
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
    virtual void _validate_property(PropertyInfo &prop) const;
	static void _bind_methods();
    bool _sort_labels(Variant a, Variant b) const;

public:
    FlashPlayer();

    float get_frame() const { return frame; }
    void set_frame(float p_frame) { frame = p_frame; update(); }
    void override_frame(String p_symbol, Variant p_frame);
    void set_variant(String key, Variant value);
    String get_variant(String key) const;
    float get_symbol_frame(String p_symbol, float p_default);
    float get_frame_rate() const { return frame_rate; }
    void set_frame_rate(float p_frame_rate) { frame_rate = p_frame_rate; }
    bool is_playing() const { return playing; }
    void set_playing(bool p_playing) { playing = p_playing; }
    bool is_loop() const { return loop; }
    void set_loop(bool p_loop) { loop = p_loop; }
    Ref<FlashDocument> get_resource() const;
    void set_resource(const Ref<FlashDocument> &doc);
    float get_duration(String timeline=String(), String label=String());
    String get_active_timeline() const;
    void set_active_timeline(String p_timeline);
    String get_active_label() const;
    void set_active_label(String p_label);

    //batcher part
    void batch();
    void add_polygon(Vector<Vector2> p_points, Vector<Color> p_colors, Vector<Vector2> p_uvs);
    void update_clipping_data();
    
    void mask_begin(int layer);
    bool is_masking();
    void mask_add(Transform2D p_transform, Rect2i p_texture_region);
    void mask_end(int layer);

    void clip_begin(int layer);
    void clip_end(int layer);

    // void pop_clipping_item() { clipping_items.pop_back(); }
    // int get_clipping_depth() const { return cliping_depth; }
    // bool is_clipping() const { return cliping_depth > 0; }
    // void add_clipping_depth() { cliping_depth++; }
    // void drop_clipping_depth() { cliping_depth--; }

};


#endif
#endif