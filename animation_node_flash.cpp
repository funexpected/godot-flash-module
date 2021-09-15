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

template <class T>
T* AnimationStateBaseNode::editor_get_state_root() const {
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor) return NULL;

    AnimationTree *tree = editor->get_tree();
    if (!tree || !tree->has_node(tree->get_animation_player())) return NULL;
    
    AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(tree->get_node(tree->get_animation_player()));
    if (!ap) return NULL;

    T *root = Object::cast_to<T>(ap->get_node(ap->get_root()));
    return root;
#else
    return NULL;
#endif
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
    } else {
        return false;
    }
}

void AnimationNodeFlashSymbol::_get_property_list(List<PropertyInfo> *p_list) const {
    FlashPlayer *fp = editor_get_state_root<FlashPlayer>();
    if (!fp) return;
    
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

	AnimationPlayer *ap = state->player;
	ERR_FAIL_COND_V(!ap, 0);
    FlashPlayer *fp = Object::cast_to<FlashPlayer>(ap->get_node(ap->get_root()));
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
    if (p_name == "flash_track") {
        track = p_value;
        _change_notify("flash_track");
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

bool AnimationNodeFlashClip::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "flash_track") {
        r_ret = track;
        return true;
    } else if (p_name == "flash_clip") {
        r_ret = clip;
        return true;
    } else {
        return false;
    }
}

void AnimationNodeFlashClip::_get_property_list(List<PropertyInfo> *p_list) const {
    FlashPlayer *fp = editor_get_state_root<FlashPlayer>();
    if (!fp) return;

    PoolStringArray tracks = fp->get_clips_tracks();
    if (track == "" || track == "[select]") tracks.insert(0, "[select]");
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_track", PROPERTY_HINT_ENUM, tracks.join(",")));
    if (track == "" || track == "[default]") return;

    PoolStringArray clips = fp->get_clips_for_track(track);
    clips.insert(0, "[default]");
    p_list->push_back(PropertyInfo(Variant::STRING, "flash_clip", PROPERTY_HINT_ENUM, clips.join(",")));
}

String AnimationNodeFlashClip::get_configuration_warning() const {
    if (track == "" || track == "[select]") {
        return "Track not selected";
    } else {
        return "";
    }
}

void AnimationNodeFlashClip::get_parameter_list(List<PropertyInfo> *r_list) const {
	r_list->push_back(PropertyInfo(Variant::REAL, time, PROPERTY_HINT_NONE, "", 0));
}

String AnimationNodeFlashClip::get_caption() const {
	return "FlashSwitchClip";
}

float AnimationNodeFlashClip::process(float p_time, bool p_seek) {
    if (track == "" || track == "[select]") return 0.0;

	AnimationPlayer *ap = state->player;
	ERR_FAIL_COND_V(!ap, 0);
    FlashPlayer *fp = Object::cast_to<FlashPlayer>(ap->get_node(ap->get_root()));
    ERR_FAIL_COND_V(!fp, 0);

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
    fp->advance_clip_for_track(track, clip, step, p_seek, &elapsed, &remaining);
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
    if (p_name == "state_property") {
        StringName value = p_value;
        if (state_property != value) {
            state_property = value;
            state_value = Variant();

            _change_notify("state_property");
            property_list_changed_notify();
        }
        return true;
    } else if (p_name == "state_value") {
        _change_notify("state_value");
        state_value = p_value;
        return true;
    } else {
        return false;
    }
}

bool AnimationNodeStateUpdate::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "state_property") {
        r_ret = state_property;
        return true;
    } else if (p_name == "state_value") {
        r_ret = state_value;
        return true;
    } else {
        return false;
    }
}

void AnimationNodeStateUpdate::_get_property_list(List<PropertyInfo> *p_list) const {
    Node *target = editor_get_state_root<Node>();
    if (!target) return;

    Vector<String> state_prop_names;
    List<PropertyInfo> state_props;
    target->get_property_list(&state_props);
    PropertyInfo state_value_info;

    for (List<PropertyInfo>::Element *E = state_props.front(); E; E = E->next()) {
        if (!E->get().name.begins_with("state/")) {
            continue;
        }
        String prop_name = E->get().name.substr(strlen("state/"));
        state_prop_names.push_back(prop_name);
        if (prop_name == state_property) {
            state_value_info = E->get();
        }
    }

    if (state_property == "" || state_property == "[select]") state_prop_names.insert(0, "[select]");
    p_list->push_back(PropertyInfo(Variant::STRING, "state_property", PROPERTY_HINT_ENUM, String(",").join(state_prop_names)));

    if (state_property == "" || state_property == "[select]") return;
    p_list->push_back(PropertyInfo(state_value_info.type, "state_value", state_value_info.hint, state_value_info.hint_string));
    Variant default_value = Variant();
    if (state_value_info.type == Variant::STRING) {
        default_value = state_value_info.hint_string.split(",")[0];
    } else {
        Variant::CallError ce;
        default_value = Variant::construct(state_value_info.type, NULL, 0, ce);
    }
    MessageQueue::get_singleton()->push_call(get_instance_id(), "_set_default_property_value", default_value);
}

void AnimationNodeStateUpdate::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_set_default_property_value"), &AnimationNodeStateUpdate::_set_default_property_value);
}

void AnimationNodeStateUpdate::_set_default_property_value(Variant p_value) {
    if (state_value == Variant()) {
        state_value = p_value;
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

    target->set(String("state/") + state_property, state_value);

    return 0.0;
}

AnimationNodeStateUpdate::AnimationNodeStateUpdate() {
	time = "time";
}

#endif // MODULE_FLASH_ENABLED