
#ifdef MODULE_FLASH_ENABLED
#ifndef FLASH_RESOURCES_H
#define FLASH_RESOURCES_H

#include <core/resource.h>
#include <core/io/xml_parser.h>
#include <scene/resources/texture.h>

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
        add(Color()), 
        mult(Color(1,1,1,1)) {}

    inline FlashColorEffect interpolate(FlashColorEffect effect, float amount) {
        FlashColorEffect new_effect;
        new_effect.mult = mult.linear_interpolate(effect.mult, amount);
        new_effect.add = mult.linear_interpolate(effect.add, amount);
        return new_effect;
    }

    inline FlashColorEffect operator *(FlashColorEffect effect) {
        FlashColorEffect new_effect;
        new_effect.mult = mult * effect.mult;
        new_effect.add = add * effect.mult + effect.add;
        return new_effect;
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
    Transform2D parse_transform(Ref<XMLParser> parser);
    
    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);

    template <class T>
    void setup(FlashDocument *p_document, FlashElement *p_parent, List<Ref<T>> *children);
    template <class T>
    void setup(List<Ref<T>> *children);
    virtual Error parse(Ref<XMLParser> parser);
};

class FlashDocument: public FlashElement {
    GDCLASS(FlashDocument, FlashElement);

    String document_path;
    Dictionary symbols;
    Dictionary bitmaps;
    float frame_size;
    List <Ref<FlashTimeline>> timelines;
    int last_eid;

public:
    FlashDocument(): 
        frame_size(1.0/24.0),
        last_eid(0),
        document_path("") {}
    
    static void _bind_methods();

    template <class T> Ref<T> element(FlashElement *parent = NULL);

    static Ref<FlashDocument> from_file(const String &p_path);
    Error load_file(const String &path);

    String get_document_path() const { return document_path; }
    Dictionary get_symbols() const { return symbols; }
    void set_symbols(Dictionary p_symbols) { symbols = p_symbols; }
    Dictionary get_bitmaps() const { return bitmaps; }
    void set_bitmaps(Dictionary p_bitmaps) { bitmaps = p_bitmaps; }
    Array get_timelines();
    void set_timelines(Array p_timelines);
    float get_duration(String timeline = String(), String label = String());
    
    Ref<FlashTimeline> load_symbol(const String &symbol_name);
    Ref<Texture> load_bitmap(const String &bitmap_name);
    inline float get_frame_size() const { return frame_size; }
    Ref<FlashTimeline> get_main_timeline();

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> parser);
    void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());

};


class FlashBitmapItem: public FlashElement {
    GDCLASS(FlashBitmapItem, FlashElement);
    String name;
    String bitmap_path;
    Ref<Texture> texture;
public:
    FlashBitmapItem():
        name(""), 
        bitmap_path("") {}

    static void _bind_methods();
    
    String get_name() const { return name; };
    String get_bitmap_path() const { return bitmap_path; };
    Ref<Texture> load();
    Ref<Texture> get_texture() const { return texture; };
    void set_texture(Ref<Texture> p_texture) { texture = p_texture; };

    virtual Error parse(Ref<XMLParser> xml);


};

class FlashTimeline: public FlashElement {
    GDCLASS(FlashTimeline, FlashElement);
    friend FlashDocument;

    String name;
    int duration;
    Dictionary labels;
    List<Ref<FlashLayer>> layers;

public:
    FlashTimeline():
        duration(0),
        name(""){}

    static void _bind_methods();

    String get_name() const { return name; };
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    Dictionary get_labels() const { return labels; }
    void set_labels(Dictionary p_labels) { labels = p_labels; }
    Array get_layers();
    void set_layers(Array p_layers);

    void add_label(String name, float start, float duration);
    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashLayer: public FlashElement {
    GDCLASS(FlashLayer, FlashElement);
    friend FlashFrame;
    
    String name;
    String type;
    Color color;
    int duration;
    List<Ref<FlashFrame>> frames;

public:
    FlashLayer():
        name(""),
        type(""),
        duration(0),
        color(Color()){}

    static void _bind_methods();

    String get_name() const;
    String get_type() const { return type; }
    void set_type(String p_type) { type = p_type; }
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    Array get_frames();
    void set_frames(Array p_frames);

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());

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
    virtual void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashFrame: public FlashElement {
    GDCLASS(FlashFrame, FlashElement);
    friend FlashLayer;

    int index;
    int duration;
    String name;
    String label_type;
    String keymode;
    String tween_type;

    List<Ref<FlashDrawing>> elements;
    List<Ref<FlashTween>> tweens;

public:
    FlashFrame():
        index(0),
        duration(1),
        name(""),
        label_type(""),
        keymode(""),
        tween_type("none"){}

    static void _bind_methods();

    int get_index() const { return index; };
    void set_index(int p_index) { index = p_index; }
    int get_duration() const { return duration; }
    void set_duration(int p_duration) { duration = p_duration; }
    String get_name() const { return name; }
    void set_name(String p_name) { name = p_name; }
    String get_label_type() const { return label_type; }
    void set_label_type(String p_label_type) { label_type = p_label_type; }
    String get_keymode() const { return keymode; }
    void set_keymode(String p_keymode) { keymode = p_keymode; }
    String get_tween_type() const { return tween_type; }
    void set_tween_type(String p_tween_type) { tween_type = p_tween_type; }
    Array get_elements();
    void set_elements(Array p_elements);
    Array get_tweens();
    void set_tweens(Array p_tweens);

    virtual void setup(FlashDocument *p_document, FlashElement *p_parent);
    virtual Error parse(Ref<XMLParser> xml);
    void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());

     
};

class FlashInstance: public FlashDrawing {
    GDCLASS(FlashInstance, FlashDrawing);

    Vector2 center_point;
    Vector2 transformation_point;
    int first_frame;
    String loop;
    String library_item_name;

    Ref<FlashTimeline> timeline;

public:
    FlashColorEffect color_effect;
    FlashInstance():
        center_point(Vector2()),
        transformation_point(Vector2()),
        first_frame(0),
        loop("loop"),
        color_effect(FlashColorEffect()){}

    static void _bind_methods();

    int get_first_frame() const { return first_frame; }
    void set_first_frame(int p_first_frame) { first_frame = p_first_frame; }
    String get_loop() const { return loop; }
    void set_loop(String p_loop) { loop = p_loop; }
    PoolColorArray get_color_effect() const;
    void set_color_effect(PoolColorArray p_color_effect);
    String get_library_item_name() const { return library_item_name; }
    void set_library_item_name(String p_name) { library_item_name = p_name; }
    
    Ref<FlashTimeline> get_timeline();
    virtual Error parse(Ref<XMLParser> xml);
    virtual void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
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
    virtual void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};


class FlashBitmapInstance: public FlashDrawing {
    GDCLASS(FlashBitmapInstance, FlashDrawing);

    String library_item_name;

public:
    FlashBitmapInstance():
        library_item_name(""){}

    static void _bind_methods();

    String get_library_item_name() const { return library_item_name; }
    void set_library_item_name(String p_library_item_name) { library_item_name = p_library_item_name; }

    Error parse(Ref<XMLParser> xml);
    virtual void draw(FlashPlayer* node, float time, Transform2D tr=Transform2D(), FlashColorEffect effect=FlashColorEffect());
};

class FlashTween: public FlashElement {
    GDCLASS(FlashTween, FlashElement);

    String target;
    PoolVector2Array points;

public:
    FlashTween():
        target("all"){}

    static void _bind_methods();
    String get_target() const { return target; }
    void set_target(String p_target) { target = p_target; }
    PoolVector2Array get_points() const { return points; }
    void set_points(PoolVector2Array p_points) { points = p_points; }
  
    Error parse(Ref<XMLParser> xml);

    float interpolate(float time);
};

class ResourceFormatLoaderFlashTexture: public ResourceFormatLoaderStreamTexture {
public:
    virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};




#endif
#endif