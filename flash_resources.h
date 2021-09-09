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
#ifndef FLASH_RESOURCES_H
#define FLASH_RESOURCES_H

#include <core/resource.h>
#include <core/io/xml_parser.h>
#include <scene/resources/texture.h>
#include <scene/resources/material.h>

#include "flash_player.h"

class FlashPlayer;
class FlashDocument;
class FlashTimeline;
class FlashLayer;
class FlashFrame;
class FlashTween;

struct FlashColorEffect {
    Color add;
    Color mult;

    FlashColorEffect() :
        add(Color(0,0,0,0)),
        mult(Color(1,1,1,1)) {}

    inline FlashColorEffect interpolate(FlashColorEffect effect, float amount) {
        FlashColorEffect new_effect;
        new_effect.mult = mult.linear_interpolate(effect.mult, amount);
        new_effect.add = add.linear_interpolate(effect.add, amount);
        return new_effect;
    }

    inline FlashColorEffect operator *(FlashColorEffect effect) {
        FlashColorEffect new_effect;
        new_effect.mult = mult * effect.mult;
        new_effect.add = add * effect.mult + effect.add;
        return new_effect;
    }

    inline bool is_empty() const {
        return add == Color(0,0,0,0) && mult == Color(1,1,1,1);
    }
};

class FlashElement: public Resource {
    GDCLASS(FlashElement, Resource);

protected:
    FlashDocument *document;
    FlashElement *parent;
    int eid;

public:
    FlashDocument *get_document() const;
    void set_document(FlashDocument *p_document);
    FlashElement *get_parent() const;
    void set_parent(FlashElement *parent);
    int get_eid() const { return eid; }
    void set_eid(int p_eid) { eid = p_eid; }
    template <class T> T* find_parent() const;

    static void _bind_methods();
    template <class T>  Ref<T> add_child(Ref<XMLParser> parser, List< Ref<T> > *elements = NULL);
    Color parse_color(const String &p_color) const;
    FlashColorEffect parse_color_effect(Ref<XMLParser> parser) const;
    Transform2D parse_transform(Ref<XMLParser> parser);

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);

    template <class T>
    void setup(FlashDocument *p_document, FlashElement *p_parent, List<Ref<T>> *children);
    template <class T>
    void setup(List<Ref<T>> *children);
    virtual Error parse(Ref<XMLParser> parser);
};

class FlashTextureRect: public Resource {
    GDCLASS(FlashTextureRect, Resource);

    int index;
    Rect2 region;
    Rect2 margin;
    Vector2 original_size;

    static void _bind_methods();

public:
    FlashTextureRect():
        index(0),
        region(Rect2()),
        margin(Rect2()),
        original_size(Vector2()){}

	void set_index(const int p_index) { index = p_index; }
	int get_index() const { return index; }
    void set_region(const Rect2 &p_region) { region = p_region; }
	Rect2 get_region() const { return region; }
	void set_margin(const Rect2 &p_margin) { margin = p_margin; }
	Rect2 get_margin() const { return margin; }
    void set_original_size(const Vector2 &p_original_size) { original_size = p_original_size; }
	Vector2 get_original_size() const { return original_size; }
};

class FlashDocument: public FlashElement {
    GDCLASS(FlashDocument, FlashElement);

    String document_path;
    Dictionary symbols;
    Dictionary bitmaps;
    float frame_size;
    List <Ref<FlashTimeline>> timelines;
    int last_eid;
    Ref<TextureArray> atlas;
    Dictionary variants;
    int variated_symbols_count;

    static String invalid_character;

public:
    FlashDocument():
        document_path(""),
        frame_size(1.0/24.0),
        last_eid(0){}

    static void _bind_methods();

    template <class T> Ref<T> element(FlashElement *parent = NULL);
    static String validate_token(String p_token);

    static Ref<FlashDocument> from_file(const String &p_path);
    Error load_file(const String &path);

    Vector2 get_atlas_size() const;
    Ref<TextureArray> get_atlas() const { return atlas; }
    void set_atlas(Ref<TextureArray> p_atlas) { atlas = p_atlas; }
    String get_document_path() const { return document_path; }
    Dictionary get_symbols() const { return symbols; }
    void set_symbols(Dictionary p_symbols) { symbols = p_symbols; }
    Dictionary get_bitmaps() const { return bitmaps; }
    void set_bitmaps(Dictionary p_bitmaps) { bitmaps = p_bitmaps; }
    Array get_timelines();
    void set_timelines(Array p_timelines);
    int get_timelines_count() const { return timelines.size(); }
    Ref<FlashTimeline> get_timeline(int idx) const { return timelines[idx]; }
    float get_duration(String timeline = String(), String label = String());
    Dictionary get_variants() const;
    void cache_variants();
    int get_variated_symbols_count() const { return variated_symbols_count; }

    FlashTimeline* get_timeline(String token);
    void parse_timeline(const String &path);
    Ref<FlashTextureRect> get_bitmap_rect(const String &bitmap_name);
    inline float get_frame_size() const { return frame_size; }
    Ref<FlashTimeline> get_main_timeline();

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> parser);
    void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());

};

class FlashBitmapItem: public FlashElement {
    GDCLASS(FlashBitmapItem, FlashElement);
    String name;
    String bitmap_path;
    Ref<FlashTextureRect> texture;
public:
    FlashBitmapItem():
        name(""),
        bitmap_path("") {}

    static void _bind_methods();

    String get_name() const { return name; };
    String get_bitmap_path() const { return bitmap_path; };
    Ref<FlashTextureRect> get_texture() const { return texture; };
    void set_texture(Ref<FlashTextureRect> p_texture) { texture = p_texture; };

    virtual Error parse(Ref<XMLParser> xml);


};

class FlashTimeline: public FlashElement {
    GDCLASS(FlashTimeline, FlashElement);
    friend FlashDocument;

    String token;
    String local_path;
    String clips_header;
    int duration;
    Dictionary clips;
    Dictionary events;
    Dictionary variants;
    List<Ref<FlashLayer>> layers;
    List<Ref<FlashLayer>> masks;
    int variation_idx;

public:
    FlashTimeline():

        token(""),
        duration(0),
        variation_idx(-1){}

    static void _bind_methods();

    String get_token() const { return token; };
    void set_token(String p_library_name) { token = p_library_name; }
    String get_local_path() const { return local_path; }
    void set_local_path(String p_local_path) { local_path = p_local_path; }
    String get_clips_header() const { return clips_header; }
    void set_clips_header(String p_clips_header) { clips_header = p_clips_header; }
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    Dictionary get_variants() const { return variants; }
    void set_variants(Dictionary p_variants) { variants = p_variants; }
    Dictionary get_clips() const { return clips; }
    void set_clips(Dictionary p_clips) { clips = p_clips; }
    Dictionary get_events() const { return events; }
    void set_events(Dictionary p_events) { events = p_events; }
    Array get_layers();
    void set_layers(Array p_layers);
    int get_variation_idx() const { return variation_idx; }
    void set_variation_idx(int p_variation_idx) { variation_idx = p_variation_idx; }

    Ref<FlashLayer> get_layer(int idx);
    void add_label(const String &name, const String &label_type, float start, float duration);
    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashLayer: public FlashElement {
    GDCLASS(FlashLayer, FlashElement);
    friend FlashDocument;
    friend FlashFrame;

    int index;
    String layer_name;
    String type;
    int duration;
    int mask_id;
    Color color;
    List<Ref<FlashFrame>> frames;

public:
    FlashLayer():
        index(0),
        layer_name(""),
        type(""),
        duration(0),
        mask_id(0),
        color(Color()){}

    static void _bind_methods();

    int get_index() const { return index; }
    void set_index(int p_index) { index = p_index; }
    String get_layer_name() const { return layer_name; };
    void set_layer_name(String p_name) { layer_name = p_name; }
    String get_type() const { return type; }
    void set_type(String p_type) { type = p_type; }
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    int get_mask_id() const { return mask_id; }
    void set_mask_id(int p_mask_id) { mask_id = p_mask_id; }
    Array get_frames();
    void set_frames(Array p_frames);

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());

};

class FlashDrawing: public FlashElement {
    GDCLASS(FlashDrawing, FlashElement);

protected:
    Transform2D transform;

public:
    FlashDrawing(): transform(Transform2D()){}
    static void _bind_methods();
    Transform2D get_transform() const { return transform; }
    void set_transform(Transform2D p_transform) { transform = p_transform; }
    virtual void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashFrame: public FlashElement {
    GDCLASS(FlashFrame, FlashElement);
    friend FlashDocument;
    friend FlashLayer;

    int index;
    int duration;
    String frame_name;
    String label_type;
    String keymode;
    String tween_type;
    FlashColorEffect color_effect;

    List<Ref<FlashDrawing>> elements;
    List<Ref<FlashTween>> tweens;

public:
    FlashFrame():
        index(0),
        duration(1),
        frame_name(""),
        label_type(""),
        keymode(""),
        tween_type("none"){}

    static void _bind_methods();

    int get_index() const { return index; };
    void set_index(int p_index) { index = p_index; }
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    String get_frame_name() const { return frame_name; }
    void set_frame_name(String p_name) { frame_name = p_name; }
    String get_label_type() const { return label_type; }
    void set_label_type(String p_label_type) { label_type = p_label_type; }
    String get_keymode() const { return keymode; }
    void set_keymode(String p_keymode) { keymode = p_keymode; }
    String get_tween_type() const { return tween_type; }
    void set_tween_type(String p_tween_type) { tween_type = p_tween_type; }
    PoolColorArray get_color_effect() const;
    void set_color_effect(PoolColorArray p_color_effect);
    Array get_elements();
    void set_elements(Array p_elements);
    Array get_tweens();
    void set_tweens(Array p_tweens);

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());


};

class FlashInstance: public FlashDrawing {
    GDCLASS(FlashInstance, FlashDrawing);

    Vector2 center_point;
    Vector2 transformation_point;
    int first_frame;
    String loop;
    String timeline_token;
    String layer_name;

    FlashTimeline* timeline;


public:
    FlashColorEffect color_effect;
    FlashInstance():
        center_point(Vector2()),
        transformation_point(Vector2()),
        first_frame(0),
        loop("loop"),
        timeline_token(""),
        layer_name(""),
        timeline(nullptr),
        color_effect(FlashColorEffect()){}

    static void _bind_methods();

    int get_first_frame() const { return first_frame; }
    void set_first_frame(int p_first_frame) { first_frame = p_first_frame; }
    String get_loop() const { return loop; }
    void set_loop(String p_loop) { loop = p_loop; }
    PoolColorArray get_color_effect() const;
    void set_color_effect(PoolColorArray p_color_effect);
    String get_timeline_token() const { return timeline_token; }
    void set_timeline_token(String p_name) { timeline_token = p_name; }
    String get_layer_name() const { return layer_name; }

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    FlashTimeline* get_timeline();
    virtual Error parse(Ref<XMLParser> xml);
    virtual void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashShape: public FlashDrawing {
    GDCLASS(FlashShape, FlashDrawing);

public:
    virtual Error parse(Ref<XMLParser> xml);

};

class FlashGroup: public FlashDrawing {
    GDCLASS(FlashGroup, FlashDrawing);

    List<Ref<FlashDrawing>> members;

public:
    static void _bind_methods();
    Array get_members();
    void set_members(Array p_members);

    List<Ref<FlashDrawing>> all_members() const;
    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    virtual void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};


class FlashBitmapInstance: public FlashDrawing {
    GDCLASS(FlashBitmapInstance, FlashDrawing);

    String library_item_name;

    Vector<Vector2> uvs;
    Ref<FlashTextureRect> texture;

public:
    FlashBitmapInstance():
        library_item_name(""){}

    static void _bind_methods();

    Ref<FlashTextureRect> get_texture();
    String get_library_item_name() const { return library_item_name; }
    void set_library_item_name(String p_library_item_name) { library_item_name = p_library_item_name; }

    Error parse(Ref<XMLParser> xml);
    virtual void animation_process(FlashPlayer* node, float time, float delta, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashTween: public FlashElement {
    GDCLASS(FlashTween, FlashElement);
public:
    enum Method {
        NONE,
        CLASSIC,
        IN_QUAD,        OUT_QUAD,         INOUT_QUAD,
        IN_CUBIC,       OUT_CUBIC,        INOUT_CUBIC,
        IN_QUART,       OUT_QUART,        INOUT_QUART,
        IN_QUINT,       OUT_QUINT,        INOUT_QUINT,
        IN_SINE,        OUT_SINE,         INOUT_SINE,
        IN_BACK,        OUT_BACK,         INOUT_BACK,
        IN_CIRC,        OUT_CIRC,         INOUT_CIRC,
        IN_BOUNCE,      OUT_BOUNCE,       INOUT_BOUNCE,
        IN_ELASTIC,     OUT_ELASTIC,      INOUT_ELASTIC,
        CUSTOM
    };

private:
    String target;
    PoolVector2Array points;
    Method method;
    float intensity;

public:


    FlashTween():
        target("all"),
        method(NONE),
        intensity(0){}

    static void _bind_methods();
    String get_target() const { return target; }
    void set_target(String p_target) { target = p_target; }
    Method get_method() const { return method; }
    void set_method(Method p_method) { method = p_method; }
    float get_intensity() const { return intensity; }
    void set_intensity(float p_intesity) { intensity = p_intesity; }
    PoolVector2Array get_points() const { return points; }
    void set_points(PoolVector2Array p_points) { points = p_points; }

    Error parse(Ref<XMLParser> xml);

    float interpolate(float time);
};

VARIANT_ENUM_CAST(FlashTween::Method);

class ResourceFormatLoaderFlashTexture: public ResourceFormatLoader {
public:
    virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = NULL);
    virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};




#endif
#endif