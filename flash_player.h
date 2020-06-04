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
    Ref<FlashDocument> resource;
    String active_symbol;

protected:
    void _notification(int p_what);
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	static void _bind_methods();
    Ref<FlashTimeline> active_timeline;

public:
    FlashPlayer(): 
        frame(0),
        frame_rate(24),
        playing(false),
        active_symbol("[document]"){}

    float get_frame() const { return frame; }
    void set_frame(float p_frame) { frame = p_frame; update(); }
    float get_frame_rate() const { return frame_rate; }
    void set_frame_rate(float p_frame_rate) { frame_rate = p_frame_rate; }
    bool is_playing() const { return playing; }
    void set_playing(bool p_playing) { playing = p_playing; }
    Ref<FlashDocument> get_resource() const;
    void set_resource(const Ref<FlashDocument> &doc);
};


#endif
#endif