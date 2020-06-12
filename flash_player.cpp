#ifdef MODULE_FLASH_ENABLED

#include "flash_player.h"

void FlashPlayer::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE : {
            if (get_material().is_null()){
                Ref<FlashMaterial> material; material.instance();
                set_material(material);
            }
        } break;
        case NOTIFICATION_READY: {
            set_process(true);
        } break;

        case NOTIFICATION_PROCESS: {
            if (playing) {
                frame += get_process_delta_time()*frame_rate;
                if (!loop && frame > playback_end)
                    frame = playback_end - 0.0001;
                else while (frame > playback_end)
                    frame -= playback_end - playback_start;
                batch();
            }
        } break;

        case NOTIFICATION_DRAW: {
            if (active_timeline.is_valid()) {
                VisualServer::get_singleton()->canvas_item_add_triangle_array(
                    get_canvas_item(),
                    indices,
                    points,
                    colors,
                    uvs,
                    Vector<int>(),
                    Vector<float>(),
                    resource->get_atlas()->get_rid()
                );
                //active_timeline->draw(this, frame);
            }
        } break;
    }
};

// bool FlashPlayer::_set(const StringName &p_name, const Variant &p_value) {
//     if (p_name == "active_timeline") {
//         set_active_timeline(p_value);
//         return true;
//     }
//     else if (p_name == "active_label") {
//         set_active_label(p_value);
//         return true;
//     }
//     return false;
// }

// bool FlashPlayer::_get(const StringName &p_name, Variant &r_ret) const { 
//     if (p_name == "active_timeline") {
//         r_ret = active_timeline_name;
//         return true;
//     } 
//     else if (p_name == "active_label") {
//         r_ret = active_label == String() ? "[full]" : active_label;
//         return true;
//     }
//     return false;
// }

void FlashPlayer::_validate_property(PropertyInfo &prop) const {
    if (prop.name == "active_timeline"){
        String symbols_hint = "[document]";
        if (resource.is_valid()) {
            Array symbols = resource->get_symbols().keys();
            for (int i=0; i<symbols.size(); i++){
                String symbol = symbols[i];
                if (symbol.find("/") >= 0) continue;
                symbols_hint += "," + symbol;
            }
        }
        prop.hint_string = symbols_hint;
    }

    if (prop.name == "active_label") {
        String labels_hint = "[full]";
        if (active_timeline.is_valid()) {
            Array labels = active_timeline->get_labels().keys();
            labels.sort_custom((FlashPlayer*)this, "_sort_labels");
            for (int i=0; i<labels.size(); i++){
                String label = labels[i];
                labels_hint += "," + label;
            }
            prop.hint_string = labels_hint;
        }
    }

    if (prop.name == "material") {
        prop.hint_string = "FlashMaterial";
    }

}
bool FlashPlayer::_sort_labels(Variant a, Variant b) const {
    if (!active_timeline.is_valid()) return false;
    Vector2 da = active_timeline->get_labels()[a];
    Vector2 db = active_timeline->get_labels()[b];
    return da.x < db.x;
}

void FlashPlayer::set_resource(const Ref<FlashDocument> &doc) {
    if (doc != resource) active_timeline_name = "[document]";
    resource = doc;
    frame = 0;
    batched_frame = -1;
    playback_start = 0;
    playback_end = 0;
    if (resource.is_valid()) {
        active_timeline = resource->get_main_timeline();
        if (active_timeline.is_valid())
            playback_end = active_timeline->get_duration();
    }
    batch();
    _change_notify();
}

Ref<FlashDocument> FlashPlayer::get_resource() const {
    return resource;
}

void FlashPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &FlashPlayer::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &FlashPlayer::is_playing);
    ClassDB::bind_method(D_METHOD("set_loop", "loop"), &FlashPlayer::set_loop);
    ClassDB::bind_method(D_METHOD("is_loop"), &FlashPlayer::is_loop);
    ClassDB::bind_method(D_METHOD("set_frame_rate", "frame_rate"), &FlashPlayer::set_frame_rate);
    ClassDB::bind_method(D_METHOD("get_frame_rate"), &FlashPlayer::get_frame_rate);
    ClassDB::bind_method(D_METHOD("set_frame", "frame"), &FlashPlayer::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame"), &FlashPlayer::get_frame);
    ClassDB::bind_method(D_METHOD("set_resource", "resource"), &FlashPlayer::set_resource);
    ClassDB::bind_method(D_METHOD("get_resource"), &FlashPlayer::get_resource);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashPlayer::get_duration, DEFVAL(String()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("set_active_timeline", "active_timeline"), &FlashPlayer::set_active_timeline);
    ClassDB::bind_method(D_METHOD("get_active_timeline"), &FlashPlayer::get_active_timeline);
    ClassDB::bind_method(D_METHOD("set_active_label", "active_label"), &FlashPlayer::set_active_label);
    ClassDB::bind_method(D_METHOD("get_active_label"), &FlashPlayer::get_active_label);

    ClassDB::bind_method(D_METHOD("_sort_labels"), &FlashPlayer::_sort_labels);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing", PROPERTY_HINT_NONE, ""), "set_playing", "is_playing");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop", PROPERTY_HINT_NONE, ""), "set_loop", "is_loop");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "frame_rate", PROPERTY_HINT_NONE, ""), "set_frame_rate", "get_frame_rate");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "FlashDocument"), "set_resource", "get_resource");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_timeline", PROPERTY_HINT_ENUM, ""), "set_active_timeline", "get_active_timeline");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_label", PROPERTY_HINT_ENUM, ""), "set_active_label", "get_active_label");
    
}

float FlashPlayer::get_duration(String p_timeline, String p_label) {
    if (!resource.is_valid()) return 0;
    return resource->get_duration(p_timeline, p_label);
}

void FlashPlayer::set_active_timeline(String p_value) {
    if (p_value == "[document]") p_value = "";
    active_timeline_name = p_value;
    active_label = "";
    frame = 0;
    batched_frame = -1;
    playback_start = 0;
    if (resource.is_valid() && resource->get_symbols().has(active_timeline_name)) {
        active_timeline = resource->get_symbols()[active_timeline_name];
    } else if (resource.is_valid()){
        active_timeline = resource->get_main_timeline();
    } else {
        active_timeline_name = "";
    }
    
    if (active_timeline.is_valid()) {
        playback_end = active_timeline->get_duration();
    } else {
        playback_end = 0;
    }
    batch();
    _change_notify();
}
String FlashPlayer::get_active_timeline() const {
    return active_timeline_name == String() ? "[document]" : active_timeline_name;
}

void FlashPlayer::set_active_label(String p_value) {
    if (p_value == "[full]") p_value = "";
    active_label = p_value;
    if (active_timeline.is_valid()) {
        Dictionary labels = active_timeline->get_labels();
        if (labels.has(p_value)) {
            Vector2 label = labels[p_value];
            playback_start = label.x;
            playback_end = label.y;

        } else {
            playback_start = 0;
            playback_end = active_timeline->get_duration();
        }
        frame = playback_start;
        update();
    } else {
        active_label = "";
    }
}

String FlashPlayer::get_active_label() const {
    return active_label == String() ? "[full]" : active_label;
}

void FlashPlayer::batch() {
    if (batched_frame == frame)
        return;
    batched_frame = frame;
    indices.resize(0);
    points.resize(0);
    colors.resize(0);
    uvs.resize(0);
    
    if (!active_timeline.is_valid()) {
        update();
        return;
    }

    active_timeline->batch(this, frame);
    update();

}
void FlashPlayer::batch_polygon(Vector<Vector2> p_points, Vector<Color> p_colors, Vector<Vector2> p_uvs) {
    Vector<int> local_indices = Geometry::triangulate_polygon(p_points);
    for (int i=0; i<local_indices.size(); i++){
        indices.push_back(local_indices[i] + points.size());
    }
    int clipping_id = 0;
    int clipping_size = 0;
    if (clipping_items.size() > 0) {
        clipping_id = clipping_items.front()->get().idx;
        clipping_size = clipping_items.size();
    }
    for (int i=0; i<p_points.size(); i++) {
        points.push_back(p_points[i]);
        colors.push_back(p_colors[i]);
        uvs.push_back(p_uvs[i] + Vector2(clipping_id, clipping_size));
    }
}

FlashPlayer::FlashPlayer() {
    frame = 0;
    frame_rate = 24;
    playing = false;
    playback_start = 0;
    playback_end = 0;
    active_timeline_name = "[document]";
    active_label = "";
    loop = false;

    batched_frame = -1;
    cliping_depth = 0;
}
#endif