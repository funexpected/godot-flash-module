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
#ifndef ANIMATION_NODE_FLASH_H
#define ANIMATION_NODE_FLASH_H

#include "scene/animation/animation_tree.h"

class FlashMachine: public AnimationTree {
	GDCLASS(FlashMachine, AnimationTree);

	NodePath flash_player;
	String track;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;


public:
	virtual String get_configuration_warning() const;
	void set_flash_player(const NodePath &p_player);
	NodePath get_flash_player() const;
	String get_track() const { return track; }

	FlashMachine();
};

class AnimationNodeEmpty: public AnimationRootNode {
	GDCLASS(AnimationNodeEmpty, AnimationRootNode);
public:
	virtual float process(float p_time, bool p_seek);
};

class AnimationNodeDelay: public AnimationRootNode {
	GDCLASS(AnimationNodeDelay, AnimationRootNode);

	float min_delay;
	float max_delay;
	StringName time;
	StringName delay;

protected:
	static void _bind_methods();
public:
	void get_parameter_list(List<PropertyInfo> *r_list) const;
	void set_min_delay(float p_delay);
	float get_min_delay() const;
	void set_max_delay(float p_delay);
	float get_max_delay() const;
	virtual float process(float p_time, bool p_seek);

	AnimationNodeDelay();

};

class AnimationNodeFlashClip: public AnimationRootNode {
    GDCLASS(AnimationNodeFlashClip, AnimationRootNode);


	StringName symbol;
    StringName clip;
	StringName time;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _default_property_list(List<PropertyInfo> *p_list) const;

	// static void _bind_methods();

public:
	void get_parameter_list(List<PropertyInfo> *r_list) const;

	virtual String get_caption() const;
	virtual float process(float p_time, bool p_seek);

	// void set_timeline(const StringName &p_name);
	// StringName get_timeline() const;
    // void set_clip(const StringName &p_clip);
    // StringName get_clip() const;

	AnimationNodeFlashClip();
};

class AnimationNodeStateUpdate: public AnimationRootNode {
    GDCLASS(AnimationNodeStateUpdate, AnimationRootNode);

	StringName time;
	Dictionary state_update;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _default_property_list(List<PropertyInfo> *p_list) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _set_default_property_values(Dictionary values);
	static void _bind_methods();

public:
	Variant get_default_property_value() const;

	void get_parameter_list(List<PropertyInfo> *r_list) const;

	virtual String get_caption() const;
	virtual float process(float p_time, bool p_seek);

	AnimationNodeStateUpdate();
};



#endif // ANIMATION_NODE_FLASH_H
#endif // MODULE_FLASH_ENABLED
