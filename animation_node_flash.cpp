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

#include "animation_node_flash.h"
#include "flash_player.h"
#include "core/message_queue.h"
#ifdef TOOLS_ENABLED
#include "editor/plugins/animation_tree_editor_plugin.h"
#endif

String FlashMachine::get_configuration_warning() const {
    String warning = AnimationTree::get_configuration_warning();

    if (!has_node(flash_player)) {
		if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("Path to an FlashPlayer node containing flash animations is not set.");
        return warning;
	}

    FlashPlayer* fp = Object::cast_to<FlashPlayer>(get_node(flash_player));
    if (!fp) {
        if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("Path set for FlashPlayer does not lead to an FlashPlayer node.");
        return warning;
    }

    if (!fp->get_resource().is_valid()) {
        if (warning != String()) {
			warning += "\n\n";
		}
		warning += TTR("FlashPlayer node missing resource with animations.");
        return warning;
    }

    return warning;
}

void FlashMachine::set_flash_player(const NodePath &p_player) {
    flash_player = p_player;
#ifdef TOOLS_ENABLED
    property_list_changed_notify();
    update_configuration_warning();
    FlashPlayer* fp = Object::cast_to<FlashPlayer>(get_node(flash_player));
    if (fp) {
        fp->connect("resource_changed", this, "update_configuration_warning");
        fp->connect("resource_changed", this, "property_list_changed_notify");
    }
#endif
}

NodePath FlashMachine::get_flash_player() const {
    return flash_player;
}

void FlashMachine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_flash_player", "flash_player"), &FlashMachine::set_flash_player);
	ClassDB::bind_method(D_METHOD("get_flash_player"), &FlashMachine::get_flash_player);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "flash_player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "FlashPlayer"), "set_flash_player", "get_flash_player");
}


bool FlashMachine::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "track") {
        track = p_value;
        return true;
    }
    return false;
}

bool FlashMachine::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "track") {
        r_ret = track;
        return true;
    }
    return false;
}

void FlashMachine::_get_property_list(List<PropertyInfo> *p_list) const {
    if (!has_node(flash_player)) return;
    FlashPlayer* fp = Object::cast_to<FlashPlayer>(get_node(flash_player));
    if (!fp) return;
    Vector<String> tracks;
    PoolStringArray existed_tracks = fp->get_clips_tracks();
    tracks.push_back("[main]");
    for (int i=0; i < existed_tracks.size(); i++) {
        tracks.push_back(existed_tracks[i]);
    }
    p_list->push_back(PropertyInfo(Variant::STRING, "track", PROPERTY_HINT_ENUM, String(",").join(tracks)));
}

FlashMachine::FlashMachine() {
    track = "[main]";
}



float AnimationNodeEmpty::process(float p_time, bool p_seek) {
    return 0.0;
}


/*
 *          Flash Symbol
 */
bool AnimationNodeFlashSymbol::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "flash_symbol") {
        symbol = p_value;
        clip = "[full]";
        _change_notify("flash_symbol");
        property_list_changed_notify();
        return true;
    } else if (p_name == "flash_clip") {
        _change_notify("flash_clip");
        clip = p_value;
        return true;
    } else {
        return false;
    }
}

bool AnimationNodeFlashSymbol::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "flash_symbol") {
        r_ret = symbol;
        return true;
    } else if (p_name == "flash_clip") {
        r_ret = clip;
        return true;
    } else if (p_name == "warning") {
        r_ret = "";
        return true;
    } else {
        return false;
    }
}

void AnimationNodeFlashSymbol::_default_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_symbol", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_clip", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "warning", PROPERTY_HINT_PLACEHOLDER_TEXT, "Invalid flash symbol state"));
}

void AnimationNodeFlashSymbol::_get_property_list(List<PropertyInfo> *p_list) const {
    FlashPlayer *fp;
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor) return _default_property_list(p_list);

    FlashMachine *tree = Object::cast_to<FlashMachine>(editor->get_tree());
    if (!tree || !tree->has_node(tree->get_animation_player())) return _default_property_list(p_list);
    
    fp = Object::cast_to<FlashPlayer>(tree->get_node(tree->get_flash_player()));
    if (!fp) return _default_property_list(p_list);
#else
    return;
#endif

    PoolStringArray symbols = fp->get_symbols();
    if (symbol == "" || symbol == "[select]") symbols.insert(0, "[select]");

    p_list->push_back(PropertyInfo(Variant::STRING, "flash_symbol", PROPERTY_HINT_ENUM, symbols.join(",")));
    if (symbol == "" || symbol == "[select]") return;

    PoolStringArray clips = fp->get_clips(symbol);
    if (clips.size() == 0) return;
    clips.insert(0, "[default]");
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_clip", PROPERTY_HINT_ENUM, clips.join(",")));
}

void AnimationNodeFlashSymbol::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::REAL, time, PROPERTY_HINT_NONE, "", 0));
}

String AnimationNodeFlashSymbol::get_caption() const {
	return "FlashTimeline";
}

float AnimationNodeFlashSymbol::process(float p_time, bool p_seek) {
    if (symbol == "" || symbol == "[select]") return 0.0;

	FlashMachine *fm = Object::cast_to<FlashMachine>(state->tree);
	ERR_FAIL_COND_V(!fm, 0);
    FlashPlayer *fp = Object::cast_to<FlashPlayer>(fm->get_node(fm->get_flash_player()));
    ERR_FAIL_COND_V(!fp, 0);

	float time = get_parameter(this->time);

	float step = p_time;

	if (p_seek) {
		time = p_time;
        step = 0;
	} else {
		time = MAX(0, time + p_time);
	}

	float anim_size = fp->get_duration(symbol, clip) / fp->get_frame_rate();
    if (time > anim_size) {

		time = anim_size;
	}

	//blend_animation(animation, time, step, p_seek, 1.0);
    fp->set_active_symbol(symbol);
    fp->set_active_clip(clip);
    fp->advance(step, p_seek, false);

	set_parameter(this->time, time);

	return anim_size - time;
}


AnimationNodeFlashSymbol::AnimationNodeFlashSymbol() {
	time = "time";
}






/*
 *          Flash Clip
 */

bool AnimationNodeFlashClip::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "clip") {
        clip = p_value;
        return true;
    } else if (p_name == "flash_symbol") {
        old_symbol = p_value;
        return true;
    } else if (p_name == "flash_track") {
        old_track = p_value;
        _change_notify("flash_track");
        property_list_changed_notify();
        return true;
    } else if (p_name == "flash_clip") {
        _change_notify("flash_clip");
        old_clip = p_value;
        return true;
    } else {
        return false;
    }
}

bool AnimationNodeFlashClip::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "clip") {
        r_ret = clip;
        return true;
    } else if (p_name == "flash_symbol" ) {
        r_ret = old_symbol;
        return true;
    } else if (p_name == "flash_track") {
        r_ret = old_track;
        return true;
    } else if (p_name == "flash_clip") {
        r_ret = old_clip;
        return true;
    } else if (p_name == "warning") {
        r_ret = "";
        return true;
    } else {
        return false;
    }
}

void AnimationNodeFlashClip::_default_property_list(List<PropertyInfo> *p_list) const {
    p_list->push_back(PropertyInfo(Variant::STRING, "clip", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_track", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_clip", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "warning", PROPERTY_HINT_PLACEHOLDER_TEXT, "Invalid flash symbol state"));
}

void AnimationNodeFlashClip::_get_property_list(List<PropertyInfo> *p_list) const {
    FlashPlayer *fp;
    FlashMachine *tree;
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor) return _default_property_list(p_list);

    tree = Object::cast_to<FlashMachine>(editor->get_tree());
    if (!tree || !tree->has_node(tree->get_animation_player())) return _default_property_list(p_list);
    
    fp = Object::cast_to<FlashPlayer>(tree->get_node(tree->get_flash_player()));
    if (!fp) return _default_property_list(p_list);
#else
    return;
#endif

    // legacy track/clip system
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_symbol", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_track", PROPERTY_HINT_NONE, ""));
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_clip", PROPERTY_HINT_NONE, ""));

    StringName track = tree->get_track();
    Vector<String> clips;
    StringName default_value;
    if (track == "[main]") {
        p_list->push_back(PropertyInfo(Variant::STRING, "Track: [main]", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));
        PoolStringArray symbols = fp->get_symbols();
        for (int i=0; i < symbols.size(); i++) {
            String symbol = symbols[i];
            if (i == 0) default_value = symbol;
            PoolStringArray symbol_clips = fp->get_clips(symbol);
            clips.push_back(symbol);
            for (int j=0; j < symbol_clips.size(); j++) {
                clips.push_back(symbol + "/" + symbol_clips[j]);
            }
        }
    } else {
        p_list->push_back(PropertyInfo(Variant::STRING, String("Track: ") + track, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
        PoolStringArray track_clips = fp->get_clips_for_track(track);
        for (int i=0; i< track_clips.size(); i++) {
            if (i == 0) default_value = symbol;
            clips.push_back(track_clips[i]);
        }
    }
    p_list->push_back(PropertyInfo(Variant::STRING, "clip", PROPERTY_HINT_ENUM, String(",").join(clips)));

    if (clip == StringName()) {
        MessageQueue::get_singleton()->push_call(get_instance_id(), "set", "clip", default_value);
    }
}

void AnimationNodeFlashClip::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::REAL, time, PROPERTY_HINT_NONE, "", 0));
}

String AnimationNodeFlashClip::get_caption() const {
	return "FlashSwitchClip";
}

float AnimationNodeFlashClip::process(float p_time, bool p_seek) {
    if (clip == StringName()) return 0.0;

    FlashMachine *fm = Object::cast_to<FlashMachine>(state->tree);
    ERR_FAIL_COND_V(!fm, 0);
    FlashPlayer *fp = Object::cast_to<FlashPlayer>(fm->get_node(fm->get_flash_player()));
    ERR_FAIL_COND_V(!fp, 0);
    StringName track = fm->get_track();

	float time = get_parameter(this->time);

	float step = p_time;

	if (p_seek) {
		time = p_time;
		step = 0;
	} else {
		time = MAX(0, time + p_time);
	}
    float elapsed;
    float remaining;
    if (track == "[main]") {
        Vector<String> clip_path = String(clip).split("/", true, 1);
        String s = clip_path[0];
        String c = clip_path.size() == 2 ? clip_path[1] : "";
        float anim_size = fp->get_duration(s, c) / fp->get_frame_rate();
        if (time > anim_size) {
		    elapsed = anim_size;
	    } else {
            elapsed = time;
        }
        remaining = anim_size - elapsed;
        fp->set_active_symbol(s);
        fp->set_active_clip(c);
        fp->advance(step, p_seek, false);
    } else {
        fp->advance_clip_for_track(track, clip, step, p_seek, &elapsed, &remaining);
    }
	set_parameter(this->time, elapsed);
	return remaining;
}

AnimationNodeFlashClip::AnimationNodeFlashClip() {
	time = "time";
}






/*
 *          State Update
 */

bool AnimationNodeStateUpdate::_set(const StringName &p_name, const Variant &p_value) {
    String name = p_name;
    if (p_name == "action/add") {
        state_update[p_value] = Variant();
        property_list_changed_notify();
        return true;
    } else if (p_name == "action/remove") {
        state_update.erase(p_value);
        property_list_changed_notify();
        return true;
    } else if (name.begins_with("update/")) {
        // print_line(String("_set state update ") + p_name + " " + p_value));
        state_update[name.replace_first("update/", "")] = p_value;
        return true;
    }
    return false;
    
}

bool AnimationNodeStateUpdate::_get(const StringName &p_name, Variant &r_ret) const {
    String name = p_name;
    if (name.begins_with("update/")) {
        String prop_name = name.replace_first("update/", "");
        if (state_update.has(prop_name)) {
            r_ret = state_update[prop_name];
            return true;
        }
    }
    return false;
}

void AnimationNodeStateUpdate::_default_property_list(List<PropertyInfo> *p_list) const {
    for (int i=0; i<state_update.size(); i++) {
        String prop_name = state_update.get_key_at_index(i);
        Variant prop_value = state_update.get_value_at_index(i);
        p_list->push_back(PropertyInfo(prop_value.get_type(), String("update/") + prop_name));
    }
}


void AnimationNodeStateUpdate::_get_property_list(List<PropertyInfo> *p_list) const {
    Node* target;
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor) return _default_property_list(p_list);

    AnimationTree *tree = Object::cast_to<AnimationTree>(editor->get_tree());
    if (!tree || !tree->has_node(tree->get_animation_player())) return _default_property_list(p_list);
    
    AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(tree->get_node(tree->get_animation_player()));
    if (!ap) return _default_property_list(p_list);

    target = Object::cast_to<Node>(ap->get_node(ap->get_root()));
    if (!target) return _default_property_list(p_list);
#else
    return;
#endif

    HashMap<String, PropertyInfo> state_props;
    List<PropertyInfo> state_props_list;
    target->get_property_list(&state_props_list);
    Array defined_prop_names = state_update.keys();
    Dictionary default_values;

    for (List<PropertyInfo>::Element *E = state_props_list.front(); E; E = E->next()) {
        if (!E->get().name.begins_with("state/")) {
            continue;
        }
        String prop_name = E->get().name.substr(strlen("state/"));
        PropertyInfo prop_info = E->get();
        state_props.set(prop_name, prop_info);
        Variant default_value;
        if (prop_info.type == Variant::STRING) {
            default_value = prop_info.hint_string.split(",")[0];
        } else {
            Variant::CallError ce;
            default_value = Variant::construct(prop_info.type, NULL, 0, ce);
        }
        default_values[prop_name] = default_value;
    }

    
    for (int i=0; i<defined_prop_names.size(); i++) {
        String prop_name = defined_prop_names[i];
        if (!state_props.has(prop_name)) {
            continue;
        }
        PropertyInfo prop_info = state_props[prop_name];
        p_list->push_back(PropertyInfo(prop_info.type, String("update/") + prop_name, prop_info.hint, prop_info.hint_string));
    }

    String add_props_hint = "[select]";
    String remove_props_hint = "[select]";
    for (List<PropertyInfo>::Element *E = state_props_list.front(); E; E = E->next()) {
        if (!E->get().name.begins_with("state/")) {
            continue;
        }
        String prop_name = E->get().name.substr(strlen("state/"));
        if (!state_props.has(prop_name)) {
            continue;
        }
        if (defined_prop_names.has(prop_name)) {
            remove_props_hint += "," + prop_name;
        } else {
            add_props_hint += "," + prop_name;
        }
    }
    if (add_props_hint != "[select]") {
        p_list->push_back(PropertyInfo(Variant::STRING, "action/add", PROPERTY_HINT_ENUM, add_props_hint));
    }
    if (remove_props_hint != "[select]") {
        p_list->push_back(PropertyInfo(Variant::STRING, "action/remove", PROPERTY_HINT_ENUM, remove_props_hint));
    }

    MessageQueue::get_singleton()->push_call(get_instance_id(), "_set_default_property_values", default_values);
}

void AnimationNodeStateUpdate::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_set_default_property_values"), &AnimationNodeStateUpdate::_set_default_property_values);
}

void AnimationNodeStateUpdate::_set_default_property_values(Dictionary default_values) {
    Array defined_values = state_update.keys();
    for (int i=0; i<defined_values.size(); i++) {
        String prop_name = defined_values[i];
        if (!default_values.has(prop_name)) {
            continue;
        }
        Variant value = state_update[prop_name];
        if (value == Variant()) {
            state_update[prop_name] = default_values[prop_name];
        }
    }
}

void AnimationNodeStateUpdate::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::REAL, time, PROPERTY_HINT_NONE, "", 0));
}

String AnimationNodeStateUpdate::get_caption() const {
	return "StateUpdate";
}

float AnimationNodeStateUpdate::process(float p_time, bool p_seek) {
	AnimationPlayer *ap = state->player;
	ERR_FAIL_COND_V(!ap, 0);
    Node *target = Object::cast_to<Node>(ap->get_node(ap->get_root()));
    ERR_FAIL_COND_V(!target, 0);
    for (int i=0; i<state_update.size(); i++) {
        target->set(String("state/") + state_update.get_key_at_index(i), state_update.get_value_at_index(i));
    }

    return 0.0;
}

AnimationNodeStateUpdate::AnimationNodeStateUpdate() {
	time = "time";
}

#endif // MODULE_FLASH_ENABLED