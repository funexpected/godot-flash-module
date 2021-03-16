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

#include "flash_resources.h"
#include "core/io/compression.h"
#include "core/io/marshalls.h"

FlashDocument *FlashElement::get_document() const {
    return document;
}
void FlashElement::set_document(FlashDocument *p_document) {
    document = p_document;
}
FlashElement *FlashElement::get_parent() const {
    return parent;
}
void FlashElement::set_parent(FlashElement *p_parent) {
    parent = p_parent;
}
template <class T> T* FlashElement::find_parent() const {
    FlashElement *ptr = parent;
    while (ptr != NULL) {
        T* obj = Object::cast_to<T>(ptr);
        if (obj != NULL) {
            return obj;
        }
        ptr = ptr->parent;
    }
    return NULL;
}
void FlashElement::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_eid"), &FlashElement::get_eid);
    ClassDB::bind_method(D_METHOD("set_eid", "path"), &FlashElement::set_eid);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "eid", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_eid", "get_eid");
}
void FlashElement::setup(FlashDocument *p_document, FlashElement *p_parent) {
    document = p_document;
    parent = p_parent;
}

template <class T>  Ref<T> FlashElement::add_child(Ref<XMLParser> parser, List< Ref<T> > *elements) {
    Ref<T> elem = document->element<T>(this);
    if (elements != NULL) elements->push_back(elem);
    Error err = elem->parse(parser);
    if (err == Error::ERR_SKIP) {
        if (elements != NULL) elements->erase(elem);
        elem.unref();
    }
    return elem;
}

Color FlashElement::parse_color(const String &p_color) const {
    return Color::html(p_color.substr(1, p_color.length()));
}

FlashColorEffect FlashElement::parse_color_effect(Ref<XMLParser> xml) const {
    FlashColorEffect color_effect;
    if (xml->has_attribute("tintColor") || xml->has_attribute("tintMultiplier")) {
            Color tint = xml->has_attribute("tintColor") ?
                parse_color(xml->get_attribute_value_safe("tintColor")) : Color(0, 0, 0, 1);
            float amount = xml->has_attribute("tintMultiplier") ?
                xml->get_attribute_value_safe("tintMultiplier").to_float() : 0.0;

            color_effect.add.r = tint.r * amount;
            color_effect.add.g = tint.g * amount;
            color_effect.add.b = tint.b * amount;
            color_effect.mult.r = 1 - amount;
            color_effect.mult.g = 1 - amount;
            color_effect.mult.b = 1 - amount;
    } else if (
            xml->has_attribute("redMultiplier")
        || xml->has_attribute("greenMultiplier")
        || xml->has_attribute("blueMultiplier")
        || xml->has_attribute("alphaMultiplier")
        || xml->has_attribute("redOffset")
        || xml->has_attribute("greenOffset")
        || xml->has_attribute("blueOffset")
        || xml->has_attribute("alphaOffset")
    ) {
        color_effect.mult.r = xml->has_attribute("redMultiplier") ? xml->get_attribute_value_safe("redMultiplier").to_float() : 1.0;
        color_effect.mult.g = xml->has_attribute("greenMultiplier") ? xml->get_attribute_value_safe("greenMultiplier").to_float() : 1.0;
        color_effect.mult.b = xml->has_attribute("blueMultiplier") ? xml->get_attribute_value_safe("blueMultiplier").to_float() : 1.0;
        color_effect.mult.a = xml->has_attribute("alphaMultiplier") ? xml->get_attribute_value_safe("alphaMultiplier").to_float() : 1.0;
        color_effect.add.r = xml->has_attribute("greenOffset") ? xml->get_attribute_value_safe("redOffset").to_float()/255.0 : 0.0;
        color_effect.add.g = xml->has_attribute("greenOffset") ? xml->get_attribute_value_safe("greenOffset").to_float()/255.0 : 0.0;
        color_effect.add.b = xml->has_attribute("blueOffset") ? xml->get_attribute_value_safe("blueOffset").to_float()/255.0 : 0.0;
        color_effect.add.a = xml->has_attribute("alphaOffset") ? xml->get_attribute_value_safe("alphaOffset").to_float()/255.0 : 0.0;
    } else if (xml->has_attribute("alphaMultiplier")) {
        color_effect.mult.a = xml->get_attribute_value_safe("alphaMultiplier").to_float();
    } else if (xml->has_attribute("brightness")) {
        float b = xml->get_attribute_value_safe("brightness").to_float();
        if (b < 0) {
            color_effect.mult.r = 1 + b;
            color_effect.mult.g = 1 + b;
            color_effect.mult.b = 1 + b;
        } else {
            color_effect.mult.r = 1 - b;
            color_effect.mult.g = 1 - b;
            color_effect.mult.b = 1 - b;
            color_effect.add.r = b;
            color_effect.add.g = b;
            color_effect.add.b = b;
        }
    }
    return color_effect;
}

Transform2D FlashElement::parse_transform(Ref<XMLParser> xml) {
    ERR_FAIL_COND_V_MSG(xml->get_node_type() != XMLParser::NODE_ELEMENT, Transform2D(), "Not matrix node");
    ERR_FAIL_COND_V_MSG(xml->get_node_name() != "Matrix", Transform2D(), "Not Matrix node");
    float tx = 0, ty = 0, a = 1, b = 0, c = 0, d = 1;
    if (xml->has_attribute("tx"))
        tx = xml->get_attribute_value_safe("tx").to_float();
    if (xml->has_attribute("ty"))
        ty = xml->get_attribute_value_safe("ty").to_float();
    if (xml->has_attribute("a"))
        a = xml->get_attribute_value_safe("a").to_float();
    if (xml->has_attribute("b"))
        b = xml->get_attribute_value_safe("b").to_float();
    if (xml->has_attribute("c"))
        c = xml->get_attribute_value_safe("c").to_float();
    if (xml->has_attribute("d"))
        d = xml->get_attribute_value_safe("d").to_float();
    return Transform2D(a, b, c, d, tx, ty);
}

Error FlashElement::parse(Ref<XMLParser> xml) {
    return Error::ERR_METHOD_NOT_FOUND;
}
void FlashDocument::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_file", "path"), &FlashDocument::load_file);
    ClassDB::bind_method(D_METHOD("get_atlas"), &FlashDocument::get_atlas);
    ClassDB::bind_method(D_METHOD("set_atlas", "atlas"), &FlashDocument::set_atlas);
    ClassDB::bind_method(D_METHOD("get_symbols"), &FlashDocument::get_symbols);
    ClassDB::bind_method(D_METHOD("set_symbols", "symbols"), &FlashDocument::set_symbols);
    ClassDB::bind_method(D_METHOD("get_bitmaps"), &FlashDocument::get_bitmaps);
    ClassDB::bind_method(D_METHOD("set_bitmaps", "bitmaps"), &FlashDocument::set_bitmaps);
    ClassDB::bind_method(D_METHOD("get_timelines"), &FlashDocument::get_timelines);
    ClassDB::bind_method(D_METHOD("set_timelines", "timelines"), &FlashDocument::set_timelines);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashDocument::get_duration, DEFVAL(String()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("get_variants"), &FlashDocument::get_variants);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "atlas", PROPERTY_HINT_RESOURCE_TYPE, "TextureArray", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_atlas", "get_atlas");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "symbols", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_symbols", "get_symbols");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "bitmaps", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_bitmaps", "get_bitmaps");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "timelines", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_timelines", "get_timelines");
}
template <class T> Ref<T> FlashDocument::element(FlashElement *parent) {
    Ref<T> elem; elem.instance();
    elem->set_eid(last_eid++);
    elem->set_parent(parent);
    elem->set_document(this);
    return elem;
}
String FlashDocument::invalid_character = ". : @ / \" ' [ ]";
String FlashDocument::validate_token(String token) {
    Vector<String> chars = FlashDocument::invalid_character.split(" ");
    for (int i = 0; i < chars.size(); i++) {
        token = token.replace(chars[i], " ");
    }
    return token;
}
Array FlashDocument::get_timelines() {
    Array l;
    for (List<Ref<FlashTimeline>>::Element *E = timelines.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashDocument::set_timelines(Array p_timelines) {
    timelines.clear();
    for (int i=0; i<p_timelines.size(); i++) {
        Ref<FlashTimeline> timeline = p_timelines[i];
        if (timeline.is_valid()) {
            timelines.push_back(timeline);
        }
    }
    setup(this, NULL);
}
float FlashDocument::get_duration(String timeline, String label) {
    Ref<FlashTimeline> tl = get_main_timeline();
    if (timeline != String() && symbols.has(timeline)) tl = symbols[timeline];
    if (label == String() || !tl->get_clips().has(label)) return tl->get_duration();
    Vector2 lb = tl->get_clips()[label];
    return lb.y - lb.x;
}
Dictionary FlashDocument::get_variants() const {
    return variants;
}
void FlashDocument::cache_variants() {
    Set<String> variated_symbols;
    for (int i=0; i<symbols.size(); i++) {
        Ref<FlashTimeline> timeline = symbols.get_value_at_index(i);
        String token = timeline->get_token();
        for (List<Ref<FlashLayer>>::Element *L = timeline->layers.front(); L; L = L->next()) {
            Ref<FlashLayer> layer = L->get();
            for (List<Ref<FlashFrame>>::Element *F = layer->frames.front(); F; F = F->next()) {
                Ref<FlashFrame> frame = F->get();
                if (frame->label_type == "anchor") {
                    Dictionary variants_by_layer;
                    if (variants.has(layer->get_layer_name())) {
                        variants_by_layer = variants[layer->get_layer_name()];
                    } else {
                        variants[layer->get_layer_name()] = variants_by_layer;
                    }
                    Dictionary symbols_by_variant;
                    if (variants_by_layer.has(frame->frame_name)) {
                        symbols_by_variant = variants_by_layer[frame->frame_name];
                    } else {
                        variants_by_layer[frame->frame_name] = symbols_by_variant;
                    }
                    symbols_by_variant[token] = frame->index;
                    variated_symbols.insert(token);
                }
            }
        }
    }
    variated_symbols_count = variated_symbols.size();
    int variant_idx = 0;
    for (Set<String>::Element *E = variated_symbols.front(); E != NULL; E = E->next()) {
        Ref<FlashTimeline> symbol = get_symbols()[E->get()];
        symbol->set_variation_idx(variant_idx);
        variant_idx++; 
    }
}
Ref<FlashDocument> FlashDocument::from_file(const String &p_path) {
    Ref<FlashDocument> doc; doc.instance();
    Error err = doc->load_file(p_path);
    ERR_FAIL_COND_V_MSG(err != Error::OK, Ref<FlashDocument>(), "Can't open " + p_path);
    return doc;
}
Vector2 FlashDocument::get_atlas_size() const {
    return atlas.is_valid() ? Vector2(atlas->get_width(), atlas->get_height()) : Vector2();
}
Error FlashDocument::load_file(const String &p_path) {
    Ref<XMLParser> xml; xml.instance();
    Error err = xml->open(p_path);
    ERR_FAIL_COND_V_MSG(err != Error::OK, err, "Can't open " + p_path);
    xml->set_meta("path", p_path);
    parent = NULL;
    document = this;
    document_path = p_path.get_base_dir();
    err = parse(xml);
    ERR_FAIL_COND_V_MSG(err != Error::OK, err, "Can't parse " + p_path);
    return OK;
}

FlashTimeline* FlashDocument::get_timeline(String token) {
    Ref<FlashTimeline> tl = symbols.get(token, Variant());
    return tl.is_valid() ? tl.ptr() : nullptr;
}

void FlashDocument::parse_timeline(const String &path) {
    String symbol_path = document_path + "/LIBRARY/" + path;
    Ref<XMLParser> xml; xml.instance();
    Error err = xml->open(symbol_path);
    Ref<FlashTimeline> timeline = element<FlashTimeline>();
    if (err != OK) {
        return;
    }
    ERR_FAIL_COND_MSG(err != Error::OK, "Can't open " + symbol_path);
    xml->set_meta("path", symbol_path);
    timeline->parse(xml);
    timeline->set_local_path(path);
    symbols[timeline->token] = timeline;
}

void FlashDocument::setup(FlashDocument *p_document, FlashElement *p_parent) {
    document = this;
    parent = NULL;

    Array symbols_array = symbols.values();
    for (int i=0; i<symbols_array.size(); i++) {
        Ref<FlashTimeline> timeline = symbols_array[i];
        if (timeline.is_valid())
            timeline->setup(this, this);
    }

    Array bitmaps_array = bitmaps.values();
    for (int i=0; i<bitmaps_array.size(); i++) {
        Ref<FlashBitmapItem> bi = bitmaps_array[i];
        if (bi.is_valid())
            bi->setup(this, this);
    }

    for (List<Ref<FlashTimeline>>::Element *E = timelines.front(); E; E = E->next()) {
        E->get()->setup(this, this);
    }

    cache_variants();
}
Ref<FlashTextureRect> FlashDocument::get_bitmap_rect(const String &p_name) {
    ERR_FAIL_COND_V_MSG(!bitmaps.has(p_name), Ref<FlashTextureRect>(), "No bitmap found for " + p_name);
    Ref<FlashBitmapItem> item = bitmaps[p_name];
    return item->get_texture();
}
Ref<FlashTimeline> FlashDocument::get_main_timeline() {
    return timelines.front() ? timelines.front()->get() : Ref<FlashTimeline>();
}
Error FlashDocument::parse(Ref<XMLParser> xml) {
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        String n = xml->get_node_name();
        if (xml->get_node_type() == XMLParser::NODE_ELEMENT && xml->get_node_name() == "DOMTimeline") {
            add_child<FlashTimeline>(xml, &timelines);
        }
        else if (xml->get_node_type() == XMLParser::NODE_ELEMENT && xml->get_node_name() == "DOMBitmapItem") {
            Ref<FlashBitmapItem> bitmap = add_child<FlashBitmapItem>(xml);
            bitmaps[bitmap->get_name()] = bitmap;
        }
        else if (xml->get_node_type() == XMLParser::NODE_ELEMENT && xml->get_node_name() == "Include" && xml->has_attribute("href")) {
            String path = xml->get_attribute_value_safe("href");
            parse_timeline(path);
        }
    }
    return Error::OK;
}
void FlashDocument::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
    for (List<Ref<FlashTimeline>>::Element *E = timelines.front(); E; E = E->next()) {
        E->get()->animation_process(node, time, delta, tr, effect);
    }
}

void FlashBitmapItem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_texture"), &FlashBitmapItem::get_texture);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &FlashBitmapItem::set_texture);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_texture", "get_texture");
}
Error FlashBitmapItem::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("name") && xml->has_attribute("href")) {
        name = xml->get_attribute_value_safe("name");
        bitmap_path = "LIBRARY/" + xml->get_attribute_value_safe("href");
        return Error::OK;
    } else {
        return Error::ERR_INVALID_DATA;
    }
}

void FlashTimeline::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_layers"), &FlashTimeline::get_layers);
    ClassDB::bind_method(D_METHOD("set_layers", "layers"), &FlashTimeline::set_layers);
    ClassDB::bind_method(D_METHOD("get_variants"), &FlashTimeline::get_variants);
    ClassDB::bind_method(D_METHOD("set_variants", "variants"), &FlashTimeline::set_variants);
    ClassDB::bind_method(D_METHOD("get_clips"), &FlashTimeline::get_clips);
    ClassDB::bind_method(D_METHOD("set_clips", "clips"), &FlashTimeline::set_clips);
    ClassDB::bind_method(D_METHOD("get_events"), &FlashTimeline::get_events);
    ClassDB::bind_method(D_METHOD("set_events", "labels"), &FlashTimeline::set_events);
    ClassDB::bind_method(D_METHOD("get_token"), &FlashTimeline::get_token);
    ClassDB::bind_method(D_METHOD("set_token", "token"), &FlashTimeline::set_token);
    ClassDB::bind_method(D_METHOD("get_local_path"), &FlashTimeline::get_local_path);
    ClassDB::bind_method(D_METHOD("set_local_path", "local_path"), &FlashTimeline::set_local_path);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashTimeline::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashTimeline::set_duration);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_layers", "get_layers");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "variants", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_variants", "get_variants");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "clips", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_clips", "get_clips");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "events", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_events", "get_events");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "token", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_token", "get_token");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "local_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_local_path", "get_local_path");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_duration", "get_duration");
}
Array FlashTimeline::get_layers() {
    Array l;
    for (List<Ref<FlashLayer>>::Element *E = masks.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    for (List<Ref<FlashLayer>>::Element *E = layers.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashTimeline::set_layers(Array p_layers) {
    layers.clear();
    for (int i=0; i<p_layers.size(); i++) {
        Ref<FlashLayer> layer = p_layers[i];
        if (layer.is_valid()) {
            if (layer->get_type() == "mask")
                masks.push_back(layer);
            else
                layers.push_back(layer);
        }
    }
}
Ref<FlashLayer> FlashTimeline::get_layer(int index) {
    for (List<Ref<FlashLayer>>::Element *E = masks.front(); E; E = E->next()) {
        if (E->get()->get_index() == index) return E->get();
    }
    for (List<Ref<FlashLayer>>::Element *E = layers.front(); E; E = E->next()) {
        if (E->get()->get_index() == index) return E->get();
    }
    return Ref<FlashLayer>();
}
void FlashTimeline::add_label(const String &name, const String &label_type, float start, float duration) {
    if (label_type == "anchor") {
        variants[name] = start;
    } else if (label_type == "comment") {
        PoolRealArray timings;
        if (events.has(name)) {
            timings = events[name];
        } else {
            events[name] = timings;
        }
        timings.push_back(start);
        events[name] = timings;
        //events[name] = Vector2(start, start+duration);
    } else {
        clips[name] = Vector2(start, start+duration);
    }
    //labels[name] = Vector2(start, start+duration);
}
void FlashTimeline::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashElement::setup(p_document, p_parent);
    for (List<Ref<FlashLayer>>::Element *E = layers.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
    for (List<Ref<FlashLayer>>::Element *E = masks.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }

}
Error FlashTimeline::parse(Ref<XMLParser> xml) {
    if (xml->is_empty()) return Error::OK;
    int layer_index = 0;
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMSymbolItem" && xml->get_node_type() == XMLParser::NODE_ELEMENT) {
            token = FlashDocument::validate_token(xml->get_attribute_value_safe("name"));
        } else if (xml->get_node_name() == "DOMTimeline") {
            if (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()){
                return Error::OK;
            } else {
                //token = FlashDocument::validate_token(xml->get_attribute_value_safe("name"));
            }
        } else if (xml->get_node_type() == XMLParser::NODE_ELEMENT && xml->get_node_name() == "DOMLayer") {
			Ref<FlashLayer> layer = add_child<FlashLayer>(xml);
            if (layer.is_null()) {
                layer_index++;
                continue;
            }
            if (layer->get_type() == "mask") {
                masks.push_back(layer);
            } else {
                layers.push_back(layer);
            }
            if (layer->get_duration() > get_duration()) set_duration(layer->get_duration());
            layer->set_index(layer_index);
            layer_index++;
        }
    }
    return Error::OK;
}
void FlashTimeline::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
    if (events.size() && delta > 0.0) {
        float event_frame_start = -2.0;
        float event_frame_end = -2.0;
        float current_frame = floor(time);
        float prev_frame = floor(time-delta);

        if (current_frame != prev_frame) {
            event_frame_start = prev_frame;
            event_frame_end = current_frame;

            for (int i=0; i<events.size(); i++) {
                String event = events.get_key_at_index(i);
                PoolRealArray timings = events.get_value_at_index(i);
                for (int j=0; j<timings.size(); j++) {
                    float timestamp = timings[j];
                    if (event_frame_start >= 0 && timestamp >= event_frame_start && timestamp < event_frame_end) {
                        node->queue_animation_event(event);
                    } else if (event_frame_start < 0 && (timestamp >= duration + event_frame_start || timestamp < event_frame_end)) {
                        node->queue_animation_event(event, true);
                    }
                }
            }
        }
    }

    for (List<Ref<FlashLayer>>::Element *E = masks.front(); E; E = E->next()) {
        E->get()->animation_process(node, time, duration, tr, effect);
    }
    for (List<Ref<FlashLayer>>::Element *E = layers.back(); E; E = E->prev()) {
        E->get()->animation_process(node, time, duration, tr, effect);
    }
}

void FlashLayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_index"), &FlashLayer::get_index);
    ClassDB::bind_method(D_METHOD("set_index", "index"), &FlashLayer::set_index);
    ClassDB::bind_method(D_METHOD("get_layer_name"), &FlashLayer::get_layer_name);
    ClassDB::bind_method(D_METHOD("set_layer_name", "layer_name"), &FlashLayer::set_layer_name);
    ClassDB::bind_method(D_METHOD("get_type"), &FlashLayer::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "type"), &FlashLayer::set_type);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashLayer::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashLayer::set_duration);
    ClassDB::bind_method(D_METHOD("get_mask_id"), &FlashLayer::get_mask_id);
    ClassDB::bind_method(D_METHOD("set_mask_id", "mask_id"), &FlashLayer::set_mask_id);
    ClassDB::bind_method(D_METHOD("get_frames"), &FlashLayer::get_frames);
    ClassDB::bind_method(D_METHOD("set_frames", "frames"), &FlashLayer::set_frames);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "index", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_index", "get_index");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "layer_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_layer_name", "get_layer_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_type", "get_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "mask_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_mask_id", "get_mask_id");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "frames", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_frames", "get_frames");
}
Array FlashLayer::get_frames() {
    Array l;
    for (List<Ref<FlashFrame>>::Element *E = frames.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashLayer::set_frames(Array p_frames) {
    frames.clear();
    for (int i=0; i<p_frames.size(); i++) {
        Ref<FlashFrame> frame = p_frames[i];
        if (frame.is_valid()) {
            frames.push_back(frame);
        }
    }
}
void FlashLayer::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashElement::setup(p_document, p_parent);
    for (List<Ref<FlashFrame>>::Element *E = frames.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
}
Error FlashLayer::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("name"))
        layer_name = xml->get_attribute_value_safe("name");
    if (xml->has_attribute("layerType"))
        type = xml->get_attribute_value_safe("layerType");
    if (type == "guide") {
        if (xml->is_empty()) return ERR_SKIP;
        while (xml->read() == OK) {
            if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
            if (xml->get_node_name() == "DOMLayer" && xml->get_node_type() == XMLParser::NODE_ELEMENT_END)
                return Error::ERR_SKIP;
        }
    }
    if (xml->has_attribute("color"))
        color = parse_color(xml->get_attribute_value_safe("color"));
    if (xml->has_attribute("parentLayerIndex")) {
        int layer_index = xml->get_attribute_value_safe("parentLayerIndex").to_int();
        FlashTimeline *tl = find_parent<FlashTimeline>();
        Ref<FlashLayer> parent_layer = tl->get_layer(layer_index);
        if (parent_layer.is_valid() && parent_layer->type == "mask") {
            mask_id = parent_layer->get_eid();
        }
    }

    if (xml->is_empty()) return Error::OK;
    while (xml->read() == OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMLayer" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "DOMFrame") {
            Ref<FlashFrame> frame = add_child<FlashFrame>(xml, &frames);
            duration += frame->get_duration();
        }
    }
    return Error::OK;
};
void FlashLayer::animation_process(FlashPlayer* node, float time, float delta, Transform2D parent_transform, FlashColorEffect parent_effect) {
    if (type == "guide") return;
    if (type == "folder") return;
    if (type == "mask") node->mask_begin(get_eid());
    if (mask_id) node->clip_begin(mask_id);

    float frame_time = time;
    while (duration > 0 && frame_time > duration) frame_time -= duration;
    int frame_idx = static_cast<int>(floor(frame_time));

    Ref<FlashFrame> current;
    Ref<FlashFrame> next;
    for (List<Ref<FlashFrame>>::Element *E = frames.front(); E; E = E->next()) {
        if (E->get()->get_index() > frame_idx) {
            break;
        }
        current = E->get();
        if (E->next()) next = E->next()->get();
    }

    if (!current.is_valid()) return;

    float interpolation = 0;
    float current_time = frame_time - current->get_index();
    if (current->tweens.size() > 0){
        Ref<FlashTween>tween = current->tweens.front()->get();
        interpolation = tween->interpolate(current_time/current->get_duration());
    }

    int idx = 0;
    for (List<Ref<FlashDrawing>>::Element *E = current->elements.front(); E; E = E->next()) {
        Ref<FlashDrawing> elem = E->get();
        Transform2D tr = elem->get_transform();
        FlashColorEffect effect = current->color_effect;
        FlashInstance *inst = Object::cast_to<FlashInstance>(elem.ptr());
        if (inst != NULL) {
            effect = effect * inst->color_effect;
        }
        FlashColorEffect next_effect = effect;

        if (next.is_valid() && next->elements.size() >= idx+1) {
            Ref<FlashDrawing> next_elem = next->elements[idx];
            Transform2D to = next_elem->get_transform();
            Vector2 x = tr[0].linear_interpolate(to[0], interpolation);
            Vector2 y = tr[1].linear_interpolate(to[1], interpolation);
            Vector2 o = tr[2].linear_interpolate(to[2], interpolation);
            tr = Transform2D(x.x, x.y, y.x, y.y, o.x, o.y);
            FlashInstance *next_inst = Object::cast_to<FlashInstance>(next_elem.ptr());
            next_effect = next->color_effect;
            if (next_inst != NULL) {
                next_effect = next_effect * next_inst->color_effect;
            }
        }
        effect = effect.interpolate(next_effect, interpolation);


        elem->animation_process(node, frame_time - current->get_index(), delta, parent_transform * tr, parent_effect*effect);
        idx++;
    }
    if (type == "mask") node->mask_end(get_eid());
    if (mask_id) node->clip_end(mask_id);
}

void FlashDrawing::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_transform"), &FlashDrawing::get_transform);
    ClassDB::bind_method(D_METHOD("set_transform", "transform"), &FlashDrawing::set_transform);

    ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM2D, "transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_transform", "get_transform");
}
void FlashDrawing::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
}

void FlashFrame::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_index"), &FlashFrame::get_index);
    ClassDB::bind_method(D_METHOD("set_index", "index"), &FlashFrame::set_index);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashFrame::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashFrame::set_duration);
    ClassDB::bind_method(D_METHOD("get_frame_name"), &FlashFrame::get_frame_name);
    ClassDB::bind_method(D_METHOD("set_frame_name", "frame_name"), &FlashFrame::set_frame_name);
    ClassDB::bind_method(D_METHOD("get_label_type"), &FlashFrame::get_label_type);
    ClassDB::bind_method(D_METHOD("set_label_type", "label_type"), &FlashFrame::set_label_type);
    ClassDB::bind_method(D_METHOD("get_keymode"), &FlashFrame::get_keymode);
    ClassDB::bind_method(D_METHOD("set_keymode", "keymode"), &FlashFrame::set_keymode);
    ClassDB::bind_method(D_METHOD("get_tween_type"), &FlashFrame::get_tween_type);
    ClassDB::bind_method(D_METHOD("set_tween_type", "tween_type"), &FlashFrame::set_tween_type);
    ClassDB::bind_method(D_METHOD("get_color_effect"), &FlashFrame::get_color_effect);
    ClassDB::bind_method(D_METHOD("set_color_effect", "color_effect"), &FlashFrame::set_color_effect);
    ClassDB::bind_method(D_METHOD("get_elements"), &FlashFrame::get_elements);
    ClassDB::bind_method(D_METHOD("set_elements", "elements"), &FlashFrame::set_elements);
    ClassDB::bind_method(D_METHOD("get_tweens"), &FlashFrame::get_tweens);
    ClassDB::bind_method(D_METHOD("set_tweens", "tweens"), &FlashFrame::set_tweens);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "index", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_index", "get_index");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "frame_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_frame_name", "get_frame_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "label_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_label_type", "get_label_type");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "keymode", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_keymode", "get_keymode");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "tween_type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_tween_type", "get_tween_type");
    ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "color_effect", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_color_effect", "get_color_effect");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elements", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_elements", "get_elements");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tweens", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_tweens", "get_tweens");
}
PoolColorArray FlashFrame::get_color_effect() const {
    PoolColorArray effect;
    if (color_effect.is_empty()) {
        return effect;
    }
    effect.push_back(color_effect.add);
    effect.push_back(color_effect.mult);
    return effect;
}
void FlashFrame::set_color_effect(PoolColorArray p_color_effect) {
    if (p_color_effect.size() > 0) {
        color_effect.add = p_color_effect[0];
    } else {
        color_effect.add = Color();
    }
    if (p_color_effect.size() > 1) {
        color_effect.mult = p_color_effect[1];
    } else {
        color_effect.mult = Color(1,1,1,1);
    }
}
Array FlashFrame::get_elements() {
    Array l;
    for (List<Ref<FlashDrawing>>::Element *E = elements.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashFrame::set_elements(Array p_elements) {
    elements.clear();
    for (int i=0; i<p_elements.size(); i++) {
        Ref<FlashDrawing> element = p_elements[i];
        if (element.is_valid())
            elements.push_back(element);
    }
}
Array FlashFrame::get_tweens() {
    Array l;
    for (List<Ref<FlashTween>>::Element *E = tweens.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashFrame::set_tweens(Array p_tweens) {
    tweens.clear();
    for (int i=0; i<p_tweens.size(); i++) {
        Ref<FlashTween> tween = p_tweens[i];
        if (tween.is_valid()) {
            tweens.push_back(tween);
        }
    }
}
void FlashFrame::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashElement::setup(p_document, p_parent);
    for (List<Ref<FlashDrawing>>::Element *E = elements.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
    for (List<Ref<FlashTween>>::Element *E = tweens.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
}
Error FlashFrame::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("index")) index = xml->get_attribute_value_safe("index").to_int();
    if (xml->has_attribute("duration")) duration = xml->get_attribute_value_safe("duration").to_int();
    if (xml->has_attribute("keymode")) keymode = xml->get_attribute_value_safe("keymode");
    if (xml->has_attribute("tweenType")) tween_type = xml->get_attribute_value_safe("tweenType");
    if (xml->has_attribute("name")) frame_name = xml->get_attribute_value_safe("name").strip_edges(true, true);
    if (xml->has_attribute("labelType")) {
        label_type = xml->get_attribute_value_safe("labelType");
    } else {
        label_type = "name";
    }
    if (xml->is_empty()) return Error::OK;
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMFrame" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            break;
        if (xml->get_node_name() == "DOMGroup")
            elements.push_back(add_child<FlashGroup>(xml));
        else if (xml->get_node_name() == "DOMShape")
            elements.push_back(add_child<FlashShape>(xml));
        else if (xml->get_node_name() == "DOMSymbolInstance")
            elements.push_back(add_child<FlashInstance>(xml));
        else if (xml->get_node_name() == "DOMBitmapInstance")
            elements.push_back(add_child<FlashBitmapInstance>(xml));
        else if (xml->get_node_name() == "CustomEase" || xml->get_node_name() == "Ease")
            tweens.push_back(add_child<FlashTween>(xml));
        else if (xml->get_node_name() == "Color")
            color_effect = parse_color_effect(xml);

    }
    if (tween_type == "motion" && tweens.size() == 0){
        Ref<FlashTween> linear = document->element<FlashTween>(this);
        tweens.push_back(linear);
    }
    if (frame_name != "") {
        FlashTimeline *tl = find_parent<FlashTimeline>();
        if (tl != NULL) {
            tl->add_label(frame_name, label_type, index, duration);
        }
    }
    return Error::OK;
}

Error FlashShape::parse(Ref<XMLParser> xml) {
    return FAILED;
    //// Even if there visible shapes in document or symbols,
    //// it should never be rendered if doc has been exported
    //// with Funexpected Flash Tools
    //// TODO: report error in `FlashShape::animation_process` calls instead
    // String layer_name = find_parent<FlashLayer>()->get_layer_name();
    // int frame_idx = find_parent<FlashFrame>()->get_index();
    // ERR_FAIL_V_MSG(FAILED, String("Vector Shape not supported at ") + xml->get_meta("path") + " in layer '" + layer_name + "' at frame " + itos(frame_idx));
}

void FlashGroup::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_members"), &FlashGroup::get_members);
    ClassDB::bind_method(D_METHOD("set_members", "members"), &FlashGroup::set_members);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "members", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_members", "get_members");

}
Array FlashGroup::get_members() {
    Array l;
    for (List<Ref<FlashDrawing>>::Element *E = members.front(); E; E = E->next()) {
        l.push_back(E->get());
    }
    return l;
}
void FlashGroup::set_members(Array p_members) {
    members.clear();
    for (int i=0; i<p_members.size(); i++) {
        Ref<FlashDrawing> member = p_members[i];
        if (member.is_valid()) {
            members.push_back(member);
        }
    }
}
List<Ref<FlashDrawing>> FlashGroup::all_members() const {
    List<Ref<FlashDrawing>> result;
    List<const FlashGroup*> groups;
    groups.push_back(this);
    for (const List<const FlashGroup*>::Element *E = groups.front(); E; E = E->next()) {
        const FlashGroup *group = E->get();
        for (const List<Ref<FlashDrawing>>::Element *M = group->members.front(); M; M = M->next()) {
            const FlashGroup *member = Object::cast_to<FlashGroup>(M->get().ptr());
            if (member) {
                groups.push_back(member);
            } else {
                result.push_back(M->get());
            }
        }
    }
    return result;
}
void FlashGroup::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashElement::setup(p_document, p_parent);
    for (List<Ref<FlashDrawing>>::Element *E = members.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
}
Error FlashGroup::parse(Ref<XMLParser> xml) {
    if (xml->is_empty()) return Error::OK;
    while (xml->read() == Error::OK){
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMGroup" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "DOMGroup" && xml->get_node_type() == XMLParser::NODE_ELEMENT)
            members.push_back(add_child<FlashGroup>(xml));
        else if (xml->get_node_name() == "DOMBitmapInstance")
            members.push_back(add_child<FlashBitmapInstance>(xml));
        else if (xml->get_node_name() == "DOMShape")
            members.push_back(add_child<FlashShape>(xml));
        else if (xml->get_node_name() == "DOMSymbolInstance")
            members.push_back(add_child<FlashShape>(xml));
    }
    return Error::OK;
}
void FlashGroup::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
    List<Ref<FlashDrawing>> ms = all_members();
    for (List<Ref<FlashDrawing>>::Element *E = ms.front(); E; E = E->next()) {
        E->get()->animation_process(node, time, delta, tr, effect);
    }
}

void FlashInstance::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_first_frame"), &FlashInstance::get_first_frame);
    ClassDB::bind_method(D_METHOD("set_first_frame", "first_frame"), &FlashInstance::set_first_frame);
    ClassDB::bind_method(D_METHOD("get_loop"), &FlashInstance::get_loop);
    ClassDB::bind_method(D_METHOD("set_loop", "loop"), &FlashInstance::set_loop);
    ClassDB::bind_method(D_METHOD("get_color_effect"), &FlashInstance::get_color_effect);
    ClassDB::bind_method(D_METHOD("set_color_effect", "color_effect"), &FlashInstance::set_color_effect);
    ClassDB::bind_method(D_METHOD("get_timeline_token"), &FlashInstance::get_timeline_token);
    ClassDB::bind_method(D_METHOD("set_timeline_token", "timeline_token"), &FlashInstance::set_timeline_token);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "first_frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_first_frame", "get_first_frame");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "loop", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_loop", "get_loop");
    ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "color_effect", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_color_effect", "get_color_effect");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "timeline_token", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_timeline_token", "get_timeline_token");
}
void FlashInstance::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashDrawing::setup(p_document, p_parent);
    layer_name = find_parent<FlashLayer>()->get_layer_name();
}
FlashTimeline* FlashInstance::get_timeline() {
    if (timeline != nullptr) return timeline;
    timeline = document->get_timeline(timeline_token);
    return timeline;
}
PoolColorArray FlashInstance::get_color_effect() const {
    PoolColorArray effect;
    effect.push_back(color_effect.add);
    effect.push_back(color_effect.mult);
    return effect;
}
void FlashInstance::set_color_effect(PoolColorArray p_color_effect) {
    if (p_color_effect.size() > 0) {
        color_effect.add = p_color_effect[0];
    } else {
        color_effect.add = Color();
    }
    if (p_color_effect.size() > 1) {
        color_effect.mult = p_color_effect[1];
    } else {
        color_effect.mult = Color(1,1,1,1);
    }
}
Error FlashInstance::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("libraryItemName")) {
        timeline_token = FlashDocument::validate_token(xml->get_attribute_value_safe("libraryItemName"));
    }
    if (xml->has_attribute("firstFrame"))
        first_frame = xml->get_attribute_value_safe("firstFrame").to_int();
    if (xml->has_attribute("loop"))
        loop = xml->get_attribute_value_safe("loop");
    if (xml->has_attribute("centerPoint3DX"))
        center_point.x = xml->get_attribute_value_safe("centerPoint3DX").to_float();
    if (xml->has_attribute("centerPoint3DY"))
        center_point.y = xml->get_attribute_value_safe("centerPoint3DY").to_float();
    if (xml->is_empty()) return Error::OK;
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMSymbolInstance" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Matrix")
            transform = parse_transform(xml);
        if (xml->get_node_name() == "Point") {
            if (xml->has_attribute("x"))
                transformation_point.x = xml->get_attribute_value_safe("x").to_float();
            if (xml->has_attribute("y"))
                transformation_point.y = xml->get_attribute_value_safe("y").to_float();
        }
        if (xml->get_node_name() == "Color") {
            color_effect = parse_color_effect(xml);
        }
    }
    return Error::OK;
}
void FlashInstance::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
    FlashTimeline* tl = get_timeline();
    if (tl == NULL) return;
    float instance_time =
        loop == "single frame"  ? first_frame :
        loop == "play once"     ? MIN(first_frame + time, tl->get_duration()-0.001) :
                                  first_frame + time;

    instance_time = node->get_symbol_frame(tl, instance_time);

    tl->animation_process(node, instance_time, delta, tr, effect);

}

void FlashBitmapInstance::_bind_methods(){
    ClassDB::bind_method(D_METHOD("get_library_item_name"), &FlashBitmapInstance::get_library_item_name);
    ClassDB::bind_method(D_METHOD("set_library_item_name", "library_item_name"), &FlashBitmapInstance::set_library_item_name);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "library_item_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_library_item_name", "get_library_item_name");
}
Error FlashBitmapInstance::parse(Ref<XMLParser> xml) {
    if(xml->has_attribute("libraryItemName"))
        library_item_name = xml->get_attribute_value_safe("libraryItemName");
    if (xml->is_empty()) return Error::OK;
    while (xml->read() == OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMBitmapInstance" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Matrix")
            transform = parse_transform(xml);
    }
    return Error::OK;
}

Ref<FlashTextureRect> FlashBitmapInstance::get_texture() {
    if (texture.is_null()) {
        texture = document->get_bitmap_rect(library_item_name);
    }
    return texture;
}

void FlashBitmapInstance::animation_process(FlashPlayer* node, float time, float delta, Transform2D tr, FlashColorEffect effect) {
    Ref<FlashTextureRect> tex = get_texture();
    if (!tex.is_valid()) {
        return;
    }
    if (node->is_masking()) {
        Transform2D scale;
        scale.scale(tex->get_original_size()/tex->get_region().size);
        node->mask_add(tr * scale, tex->get_region(), tex->get_index());
        return;
    }
    // if (node->is_masking()) {
    //     FlashClippingItem item;
    //     item.transform = tr;
    //     item.texture = document->load_bitmap(timeline_token);
    //     node->add_clipping_item(item);
    //     return;
    // }

    //node->draw_set_transform_matrix(tr);
    Vector<Color> colors;
    Color color = effect.mult * 0.5;
    color.r += floor(effect.add.r * 255);
    color.g += floor(effect.add.g * 255);
    color.b += floor(effect.add.b * 255);
    color.a += floor(effect.add.a * 255);
    colors.push_back(color);
    colors.push_back(color);
    colors.push_back(color);
    colors.push_back(color);
    Vector<Vector2> points;
    Vector2 size = tex->get_original_size();
    points.push_back(tr.xform(Vector2()));
    points.push_back(tr.xform(Vector2(size.x, 0)));
    points.push_back(tr.xform(size));
    points.push_back(tr.xform(Vector2(0, size.y)));

    if (uvs.size()  == 0) {
        Vector2 as = document->get_atlas_size();
        Rect2 r = tex->get_region();
        Vector2 start = r.position / as;
        Vector2 end = (r.position + r.size) / as;
        uvs.push_back(start);
        uvs.push_back(Vector2(end.x, start.y));
        uvs.push_back(end);
        uvs.push_back(Vector2(start.x, end.y));
    }

    node->add_polygon(points, colors, uvs, tex->get_index());
}

void FlashTween::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_target"), &FlashTween::get_target);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &FlashTween::set_target);
    ClassDB::bind_method(D_METHOD("get_intensity"), &FlashTween::get_intensity);
    ClassDB::bind_method(D_METHOD("set_intensity", "intensity"), &FlashTween::set_intensity);
    ClassDB::bind_method(D_METHOD("get_method"), &FlashTween::get_method);
    ClassDB::bind_method(D_METHOD("set_method", "method"), &FlashTween::set_method);
    ClassDB::bind_method(D_METHOD("get_points"), &FlashTween::get_points);
    ClassDB::bind_method(D_METHOD("set_points", "points"), &FlashTween::set_points);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_target", "get_target");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "intensity", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_intensity", "get_intensity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "method", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_method", "get_method");
    ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_points", "get_points");

    BIND_ENUM_CONSTANT(NONE);
    BIND_ENUM_CONSTANT(CLASSIC);
    BIND_ENUM_CONSTANT(IN_QUAD);
    BIND_ENUM_CONSTANT(OUT_QUAD);
    BIND_ENUM_CONSTANT(INOUT_QUAD);
    BIND_ENUM_CONSTANT(IN_CUBIC);
    BIND_ENUM_CONSTANT(OUT_CUBIC);
    BIND_ENUM_CONSTANT(INOUT_CUBIC);
    BIND_ENUM_CONSTANT(IN_QUART);
    BIND_ENUM_CONSTANT(OUT_QUART);
    BIND_ENUM_CONSTANT(INOUT_QUART);
    BIND_ENUM_CONSTANT(IN_QUINT);
    BIND_ENUM_CONSTANT(OUT_QUINT);
    BIND_ENUM_CONSTANT(INOUT_QUINT);
    BIND_ENUM_CONSTANT(IN_SINE);
    BIND_ENUM_CONSTANT(OUT_SINE);
    BIND_ENUM_CONSTANT(INOUT_SINE);
    BIND_ENUM_CONSTANT(IN_BACK);
    BIND_ENUM_CONSTANT(OUT_BACK);
    BIND_ENUM_CONSTANT(INOUT_BACK);
    BIND_ENUM_CONSTANT(IN_CIRC);
    BIND_ENUM_CONSTANT(OUT_CIRC);
    BIND_ENUM_CONSTANT(INOUT_CIRC);
    BIND_ENUM_CONSTANT(IN_BOUNCE);
    BIND_ENUM_CONSTANT(OUT_BOUNCE);
    BIND_ENUM_CONSTANT(INOUT_BOUNCE);
    BIND_ENUM_CONSTANT(IN_ELASTIC);
    BIND_ENUM_CONSTANT(OUT_ELASTIC);
    BIND_ENUM_CONSTANT(INOUT_ELASTIC);
    BIND_ENUM_CONSTANT(CUSTOM);
}
Error FlashTween::parse(Ref<XMLParser> xml) {
    String n = xml->get_node_name();
    if (xml->has_attribute("target"))
        target = xml->get_attribute_value_safe("target");
    if (xml->has_attribute("intensity")) {
        method = CLASSIC;
        intensity = xml->get_attribute_value_safe("intensity").to_int();
    }
    if (n == "CustomEase") {
        method = CUSTOM;
    } else if (xml->has_attribute("method")) {
        String mname = xml->get_attribute_value_safe("method");
        method =
            mname == "quadIn"       ? IN_QUINT :
            mname == "quadOut"      ? OUT_QUINT :
            mname == "quadInOut"    ? INOUT_QUINT :
            mname == "cubicIn"      ? IN_CUBIC :
            mname == "cubicOut"     ? OUT_CUBIC :
            mname == "cubicInOut"   ? INOUT_CUBIC :
            mname == "quartIn"      ? IN_QUART :
            mname == "quartOut"     ? OUT_QUART :
            mname == "quartInOut"   ? INOUT_QUART :
            mname == "quintIn"      ? IN_QUINT :
            mname == "quintOut"     ? OUT_QUINT :
            mname == "quintInOut"   ? INOUT_QUINT :
            mname == "sineIn"       ? IN_SINE :
            mname == "sineOut"      ? OUT_SINE :
            mname == "sineInOut"    ? INOUT_SINE :
            mname == "backIn"       ? IN_BACK :
            mname == "backOut"      ? OUT_BACK :
            mname == "backInOut"    ? INOUT_BACK :
            mname == "circIn"       ? IN_CIRC :
            mname == "circOut"      ? OUT_CIRC :
            mname == "circInOut"    ? INOUT_CIRC :
            mname == "bounceIn"     ? IN_BOUNCE :
            mname == "bounceOut"    ? OUT_BOUNCE :
            mname == "bounceInOut"  ? INOUT_BOUNCE :
            mname == "elasticIn"    ? IN_ELASTIC :
            mname == "elasticOut"   ? OUT_ELASTIC :
            mname == "elasticInOut" ? INOUT_ELASTIC :
                                      NONE;
    } else {
        method = CLASSIC;
    }

    if (xml->is_empty()) return Error::OK;
    while (xml->read() == OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == n && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Point") {
            Vector2 p = Vector2();
            if (xml->has_attribute("x")) p.x = xml->get_attribute_value_safe("x").to_float();
            if (xml->has_attribute("y")) p.y = xml->get_attribute_value_safe("y").to_float();
            points.push_back(p);
        }
    }
    return Error::OK;
}
static _FORCE_INLINE_ Vector2 _bezier_interp(real_t t, const Vector2 &start, const Vector2 &control_1, const Vector2 &control_2, const Vector2 &end) {
	/* Formula from Wikipedia article on Bezier curves. */
	real_t omt = (1.0 - t);
	real_t omt2 = omt * omt;
	real_t omt3 = omt2 * omt;
	real_t t2 = t * t;
	real_t t3 = t2 * t;

	return start * omt3 + control_1 * omt2 * t * 3.0 + control_2 * omt * t2 * 3.0 + end * t3;
}

inline float __ease_out_bounce(float x) {
    const float n1 = 7.5625;
    const float d1 = 2.75;
    if (x < 1 / d1) {
        return n1 * x * x;
    } else if (x < 2 / d1) {
        return n1 * pow(x - 1.5 / d1, 2) + 0.75;
    } else if (x < 2.5 / d1) {
        return n1 * pow(x - 2.25 / d1, 2) + 0.9375;
    } else {
        return n1 * pow(x - 2.625 / d1, 2) + 0.984375;
    }
}

// easing calculations taken from https://easings.net/
float FlashTween::interpolate(float time) {
    switch (method) {
        case NONE: return time;
        case CLASSIC: return Math::ease(time, intensity);
        case IN_QUAD: return time * time;
        case OUT_QUAD: return 1 - (1 - time) * (1 - time);
        case INOUT_QUAD: return time < 0.5 ? 2 * time * time : 1 - pow(-2 * time + 2, 2) / 2;
        case IN_CUBIC: return time * time * time;
        case OUT_CUBIC: return 1 - pow(1 - time, 3);
        case INOUT_CUBIC: return time < 0.5 ? 4 * time * time * time : 1 - pow(-2 * time + 2, 3) / 2;
        case IN_QUART: return pow(time, 4);
        case OUT_QUART: return 1 - pow(1 - time, 4);
        case INOUT_QUART: return time < 0.5 ? 8 * pow(time, 4) : 1 - pow(-2 * time + 2, 4) / 2;
        case IN_QUINT: return pow(time, 5);
        case OUT_QUINT: return 1 - pow(1 - time, 5);
        case INOUT_QUINT: return time < 0.5 ? 16 * pow(time, 5) : 1 - pow(-2 * time + 2, 5) / 2;
        case IN_SINE: return 1 - cos((time * Math_PI) / 2);
        case OUT_SINE: return sin((time * Math_PI) / 2);
        case INOUT_SINE: return -(cos(Math_PI * time) - 1) / 2;
        case IN_BACK: {
            const float c1 = 1.70158;
            const float c3 = c1 + 1;
            return c3 * time * time * time - c1 * time * time;
        };
        case OUT_BACK: {
            const float c1 = 1.70158;
            const float c3 = c1 + 1;
            return 1 + c3 * pow(time - 1, 3) + c1 * pow(time - 1, 2);
        };
        case INOUT_BACK: {
            const float c1 = 1.70158;
            const float c2 = c1 * 1.525;
            return time < 0.5
                ? (pow(2 * time, 2) * ((c2 + 1) * 2 * time - c2)) / 2
                : (pow(2 * time - 2, 2) * ((c2 + 1) * (time * 2 - 2) + c2) + 2) / 2;
        };
        case IN_CIRC: return 1 - sqrt(1 - pow(time, 2));
        case OUT_CIRC: return sqrt(1 - pow(time - 1, 2));
        case INOUT_CIRC: return time < 0.5
            ? (1 - sqrt(1 - pow(2 * time, 2))) / 2
            : (sqrt(1 - pow(-2 * time + 2, 2)) + 1) / 2;
        case IN_ELASTIC: {
            const float c4 = (2 * Math_PI) / 3;
            return time == 0 ? 0
                 : time == 1 ? 1
                 : -pow(2, 10 * time - 10) * sin((time * 10 - 10.75) * c4);
        }
        case OUT_ELASTIC: {
            const float c4 = (2 * Math_PI) / 3;
            return time == 0 ? 0
                 : time == 1 ? 1
                 : pow(2, -10 * time) * sin((time * 10 - 0.75) * c4) + 1;
        };
        case INOUT_ELASTIC: {
            const float c5 = (2 * Math_PI) / 4.5;
            return time == 0 ? 0
                 : time == 1 ? 1
                 : time < 0.5
                 ? -(pow(2, 20 * time - 10) * sin((20 * time - 11.125) * c5)) / 2
                 : (pow(2, -20 * time + 10) * sin((20 * time - 11.125) * c5)) / 2 + 1;
        };                                   \
        case IN_BOUNCE: return 1 - __ease_out_bounce(1 - time);
        case OUT_BOUNCE: return __ease_out_bounce(time);
        case INOUT_BOUNCE: return time < 0.5
            ? (1 - __ease_out_bounce(1 - 2 * time)) / 2
            : (1 + __ease_out_bounce(2 * time - 1)) / 2;
        case CUSTOM: {
            if (points.size() < 4) return time;
            float low = 0;
            float high = 1;
            Vector2 start;
            Vector2 start_out;
            Vector2 end_in;
            Vector2 end;

            for (int i=0; i < (points.size()-1)/3; i++){
                start = points[3*i];
                start_out = points[3*i+1];
                end_in = points[3*i+2];
                end = points[3*i+3];
                if (time < end.x) break;
            }
            //time = time - low;
            //narrow high and low as much as possible
            float middle;
            for (int i = 0; i < 10; i++) {
                middle = (low + high) / 2.0;
                Vector2 interp = _bezier_interp(middle, start, start_out, end_in, end);
                if (interp.x < time) {
                    low = middle;
                } else {
                    high = middle;
                }
            }

            //interpolate the result:
            Vector2 low_pos = _bezier_interp(low, start, start_out, end_in, end);
            Vector2 high_pos = _bezier_interp(high, start, start_out, end_in, end);

            float c = (time - low_pos.x) / (high_pos.x - low_pos.x);

            return low_pos.linear_interpolate(high_pos, c).y;
        }
        default: return time;
    }
}

void FlashTextureRect::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_index", "index"), &FlashTextureRect::set_index);
	ClassDB::bind_method(D_METHOD("get_index"), &FlashTextureRect::get_index);
	ClassDB::bind_method(D_METHOD("set_region", "region"), &FlashTextureRect::set_region);
	ClassDB::bind_method(D_METHOD("get_region"), &FlashTextureRect::get_region);
	ClassDB::bind_method(D_METHOD("set_margin", "margin"), &FlashTextureRect::set_margin);
	ClassDB::bind_method(D_METHOD("get_margin"), &FlashTextureRect::get_margin);
    ClassDB::bind_method(D_METHOD("set_original_size", "original_size"), &FlashTextureRect::set_original_size);
	ClassDB::bind_method(D_METHOD("get_original_size"), &FlashTextureRect::get_original_size);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "index"), "set_index", "get_index");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "region"), "set_region", "get_region");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "margin"), "set_margin", "get_margin");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "original_size"), "set_original_size", "get_original_size");
}

RES ResourceFormatLoaderFlashTexture::load(const String &p_path, const String &p_original_path, Error *r_error) {
	FileAccess *f = FileAccess::open(p_path, FileAccess::READ);
    int decompressed_size = f->get_32();
    int bytes;
    PoolVector<uint8_t> buff;
    buff.resize(f->get_len()-f->get_position());
    {
        PoolVector<uint8_t>::Write w = buff.write();
        bytes = f->get_buffer(w.ptr(), buff.size());
    }

    PoolVector<uint8_t> decompressed;
    decompressed.resize(decompressed_size);
    Compression::decompress(decompressed.write().ptr(), decompressed.size(), buff.read().ptr(), bytes, Compression::MODE_FASTLZ);

    PoolVector<uint8_t>::Read r = decompressed.read();
    Variant texture_info_var;
    decode_variant(texture_info_var, r.ptr(), decompressed.size(), NULL, true);
    Dictionary texture_info = texture_info_var;
    Array images = texture_info["images"];
    Ref<TextureArray> texture;
    texture.instance();
    texture->create((int)texture_info["width"], (int)texture_info["height"], images.size(), (Image::Format)(int)texture_info["format"], (int)texture_info["flags"]);

    for (int i=0; i<images.size(); i++){
        Ref<Image> img = images[i];
        texture->set_layer_data(img, i);
    }
    memdelete(f);
    return texture;
}

void ResourceFormatLoaderFlashTexture::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("ftex");
}
bool ResourceFormatLoaderFlashTexture::handles_type(const String &p_type) const {
	return p_type == "TextureArray";
}
String ResourceFormatLoaderFlashTexture::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "ftex")
		return "TextureArray";
	return "";
}
#endif