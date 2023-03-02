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

#include "animation_node_flash.h"
#include "flash_player.h"
#include "core/message_queue.h"
#include "scene/gui/control.h"
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


/*
 *          Flash Clip
 */

bool AnimationNodeFlashClip::_set(const StringName &p_name, const Variant &p_value) {
    if (p_name == "clip") {
        clip = p_value;
        return true;
    } else {
        return false;
    }
}

bool AnimationNodeFlashClip::_get(const StringName &p_name, Variant &r_ret) const {
    if (p_name == "clip") {
        r_ret = clip;
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
    p_list->push_back(PropertyInfo(Variant::STRING, "warning", PROPERTY_HINT_PLACEHOLDER_TEXT, "Invalid flash symbol state"));
}

void AnimationNodeFlashClip::_get_property_list(List<PropertyInfo> *p_list) const {
    FlashPlayer *fp;
    FlashMachine *tree;
#ifdef TOOLS_ENABLED
    AnimationTreeEditor *editor = AnimationTreeEditor::get_singleton();
    if (!editor || !editor->is_visible()) return _default_property_list(p_list);

    tree = Object::cast_to<FlashMachine>(editor->get_tree());
    if (!tree || !tree->has_node(tree->get_animation_player())) return _default_property_list(p_list);
    
    fp = Object::cast_to<FlashPlayer>(tree->get_node(tree->get_flash_player()));
    if (!fp) return _default_property_list(p_list);
#else
    return;
#endif

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
	return "Flash Clip";
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