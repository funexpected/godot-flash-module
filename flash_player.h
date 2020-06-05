#ifdef MODULE_FLASH_ENABLED
#ifndef FLASH_PLAYER_H
#define FLASH_PLAYER_H

#include <scene/2d/node_2d.h>

#include "flash_resources.h"

class FlashDocument;
class FlashTimeline;

class FlashPlayer: public Node2D {
    GDCLASS(FlashPlayer, Node2D);

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

protected:
    void _notification(int p_what);
	//bool _set(const StringName &p_name, const Variant &p_value);
	//bool _get(const StringName &p_name, Variant &r_ret) const;
	//void _get_property_list(List<PropertyInfo> *p_list) const;
    virtual void _validate_property(PropertyInfo &prop) const;
	static void _bind_methods();
    bool _sort_labels(Variant a, Variant b) const;

public:
    FlashPlayer(): 
        frame(0),
        frame_rate(24),
        playing(false),
        playback_start(0),
        playback_end(0),
        active_timeline_name("[document]"),
        active_label(""),
        loop(false){}

    float get_frame() const { return frame; }
    void set_frame(float p_frame) { frame = p_frame; update(); }
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
};


#endif
#endif