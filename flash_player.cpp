#ifdef MODULE_FLASH_ENABLED

#include "flash_player.h"

void FlashPlayer::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            set_process(true);
        } break;

        case NOTIFICATION_PROCESS: {
            if (playing) {
                frame += get_process_delta_time()*frame_rate;
                if (frame > active_timeline->get_duration())
                    frame -= active_timeline->get_duration();
                update();
            }
        } break;

        case NOTIFICATION_DRAW: {
            if (active_timeline.is_valid()) {
                active_timeline->draw(this, frame);
            }
        } break;
    }
};

bool FlashPlayer::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "active_symbol") {
        active_symbol = p_value;
        frame = 0;
        if (resource.is_valid() && resource->get_symbols().has(active_symbol))
            active_timeline = resource->get_symbols()[active_symbol];
        update();
        return true;
    }
    return false;
}

bool FlashPlayer::_get(const StringName &p_name, Variant &r_ret) const { 
    if (p_name == "active_symbol") {
        r_ret = active_symbol;
        return true;
    }
    return false;
}

void FlashPlayer::_get_property_list(List<PropertyInfo> *p_list) const {
    if (!resource.is_valid()) return;
    String symbols_hint = "[document]";
    Array symbols = resource->get_symbols().keys();
    for (int i=0; i<symbols.size(); i++){
        String symbol = symbols[i];
        if (symbol.find("/") >= 0) continue;
        symbols_hint += "," + symbol;
    }
    p_list->push_back(PropertyInfo(Variant::STRING, "active_symbol", PROPERTY_HINT_ENUM, symbols_hint));
}

void FlashPlayer::set_resource(const Ref<FlashDocument> &doc) {
    if (doc != resource) active_symbol = "[document]";
    resource = doc;
    if (resource.is_valid())
        active_timeline = resource->get_main_timeline();
    _change_notify();
}

Ref<FlashDocument> FlashPlayer::get_resource() const {
    return resource;
}

void FlashPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &FlashPlayer::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &FlashPlayer::is_playing);
    ClassDB::bind_method(D_METHOD("set_frame_rate", "frame_rate"), &FlashPlayer::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame_rate"), &FlashPlayer::get_frame_rate);
    ClassDB::bind_method(D_METHOD("set_frame", "frame"), &FlashPlayer::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame"), &FlashPlayer::get_frame);
    ClassDB::bind_method(D_METHOD("set_resource", "resource"), &FlashPlayer::set_resource);
    ClassDB::bind_method(D_METHOD("get_resource"), &FlashPlayer::get_resource);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing", PROPERTY_HINT_NONE, ""), "set_playing", "is_playing");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "frame_rate", PROPERTY_HINT_NONE, ""), "set_frame_rate", "get_frame_rate");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "FlashDocument"), "set_resource", "get_resource");
}

#endif