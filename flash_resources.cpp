
#ifdef MODULE_FLASH_ENABLED

#include "flash_resources.h"

FlashDocument *FlashElement::get_document() const {
    return document;
}

FlashElement *FlashElement::get_parent() const {
    return parent;
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
    Ref<T> elem = document->element<T>();
    //Ref<FlashElement> elem = base;
    elem->document = document;
    elem->parent = this;
    if (elements != NULL) elements->push_back(elem);
    elem->parse(parser);
    return elem;
}

Color FlashElement::parse_color(const String &p_color) const {
    return Color::html(p_color.substr(1, p_color.length()));
}

Transform2D FlashElement::parse_transform(Ref<XMLParser> xml) {
    ERR_FAIL_COND_V_MSG(xml->get_node_type() != XMLParser::NODE_ELEMENT, Transform2D(), "Not matrix node");
    ERR_FAIL_COND_V_MSG(xml->get_node_name() != "Matrix", Transform2D(), "Not Matrix node");
    float tx = 0, ty = 0, a = 1, b = 0, c = 0, d = 1;
    if (xml->has_attribute("tx"))
        tx = xml->get_attribute_value("tx").to_float();
    if (xml->has_attribute("ty"))
        ty = xml->get_attribute_value("ty").to_float();
    if (xml->has_attribute("a"))
        a = xml->get_attribute_value("a").to_float();
    if (xml->has_attribute("b"))
        b = xml->get_attribute_value("b").to_float();
    if (xml->has_attribute("c"))
        c = xml->get_attribute_value("c").to_float();
    if (xml->has_attribute("d"))
        d = xml->get_attribute_value("d").to_float();
    return Transform2D(a, b, c, d, tx, ty);
}

Error FlashElement::parse(Ref<XMLParser> xml) {
    return Error::ERR_METHOD_NOT_FOUND;
}
void FlashDocument::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_file", "path"), &FlashDocument::load_file);
    ClassDB::bind_method(D_METHOD("get_symbols"), &FlashDocument::get_symbols);
    ClassDB::bind_method(D_METHOD("set_symbols", "symbols"), &FlashDocument::set_symbols);
    ClassDB::bind_method(D_METHOD("get_bitmaps"), &FlashDocument::get_bitmaps);
    ClassDB::bind_method(D_METHOD("set_bitmaps", "bitmaps"), &FlashDocument::set_bitmaps);
    ClassDB::bind_method(D_METHOD("get_timelines"), &FlashDocument::get_timelines);
    ClassDB::bind_method(D_METHOD("set_timelines", "timelines"), &FlashDocument::set_timelines);
    

    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "symbols", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_symbols", "get_symbols");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "bitmaps", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_bitmaps", "get_bitmaps");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "timelines", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_timelines", "get_timelines");
}
template <class T> Ref<T> FlashDocument::element() {
    Ref<T> elem; elem.instance();
    elem->set_eid(last_eid++);
    return elem;
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
Ref<FlashDocument> FlashDocument::from_file(const String &p_path) {
    Ref<FlashDocument> doc; doc.instance();
    Error err = doc->load_file(p_path);
    ERR_FAIL_COND_V_MSG(err != Error::OK, Ref<FlashDocument>(), "Can't open " + p_path);
    return doc;
}
Error FlashDocument::load_file(const String &p_path) {
    Ref<XMLParser> xml; xml.instance();
    Error err = xml->open(p_path);
    ERR_FAIL_COND_V_MSG(err != Error::OK, err, "Can't open " + p_path);
    parent = NULL;
    document = this;
    document_path = p_path.get_base_dir();
    err = parse(xml);
    ERR_FAIL_COND_V_MSG(err != Error::OK, err, "Can't parse " + p_path);
    return OK;
}
Ref<FlashTimeline> FlashDocument::load_symbol(const String &symbol_name) {
    if (symbols.has(symbol_name)) {
        return symbols[symbol_name];
    }
    String symbol_path = document_path + "/LIBRARY/" + symbol_name + ".xml";
    Ref<XMLParser> xml; xml.instance();
    Error err = xml->open(symbol_path);
    if (err != OK) {
        symbols[symbol_name] = element<FlashTimeline>();
    }
    ERR_FAIL_COND_V_MSG(err != Error::OK, Ref<FlashDocument>(), "Can't open " + symbol_path);

    Ref<FlashTimeline> tl = element<FlashTimeline>();
    symbols[symbol_name] = tl;
    tl->document = document;
    tl->parent = NULL;
    tl->parse(xml);
    return tl;
}
Ref<Texture> FlashDocument::load_bitmap(const String &bitmap_name) {
    ERR_FAIL_COND_V_MSG(!bitmaps.has(bitmap_name), Ref<Texture>(), "No bitmap found for " + bitmap_name);
    Ref<FlashBitmapItem> item = bitmaps[bitmap_name];
    return item->load();
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
}
Ref<FlashTimeline> FlashDocument::get_main_timeline() {
    return timelines.front() ? timelines.front()->get() : Ref<FlashTimeline>();
}
Error FlashDocument::parse(Ref<XMLParser> xml) {
    while (xml->read() == Error::OK) {
        XMLParser::NodeType type = xml->get_node_type();
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
            String path = xml->get_attribute_value("href");
            load_symbol(path.get_basename());
        }
    }
    return Error::OK;
}
void FlashDocument::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    for (List<Ref<FlashTimeline>>::Element *E = timelines.front(); E; E = E->next()) {
        E->get()->draw(node, time, tr, effect);
    }
}

void FlashBitmapItem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_texture"), &FlashBitmapItem::get_texture);
    ClassDB::bind_method(D_METHOD("set_texture", "texture"), &FlashBitmapItem::set_texture);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_texture", "get_texture");
}
Error FlashBitmapItem::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("name") && xml->has_attribute("href")) {
        name = xml->get_attribute_value("name");
        bitmap_path = "LIBRARY/" + xml->get_attribute_value("href");
        return Error::OK;
    } else {
        return Error::ERR_INVALID_DATA;
    }
}
Ref<Texture> FlashBitmapItem::load() {
    if (texture.is_valid()) return texture;
    String texture_path = document->get_document_path() + "/" + bitmap_path;
    Ref<Image> img; img.instance();
    ERR_FAIL_COND_V_MSG(img->load(texture_path) != Error::OK, Ref<Texture>(), "Can't load image at " + texture_path);
    Ref<ImageTexture> tex; tex.instance();
    tex->create_from_image(img);
    texture = tex;
    return texture;
};

void FlashTimeline::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_layers"), &FlashTimeline::get_layers);
    ClassDB::bind_method(D_METHOD("set_layers", "layers"), &FlashTimeline::set_layers);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashTimeline::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashTimeline::set_duration);

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_layers", "get_layers");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL ), "set_duration", "get_duration");
}
Array FlashTimeline::get_layers() {
    Array l;
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
            layers.push_back(layer);
        }
    }
}
void FlashTimeline::setup(FlashDocument *p_document, FlashElement *p_parent) {
    FlashElement::setup(p_document, p_parent);
    for (List<Ref<FlashLayer>>::Element *E = layers.front(); E; E = E->next()) {
        E->get()->setup(document, this);
    }
}
Error FlashTimeline::parse(Ref<XMLParser> xml) {
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMTimeline") {
            if (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()){
                return Error::OK;
            } else {
                name = xml->get_attribute_value("name");
            }
        } else if (xml->get_node_type() == XMLParser::NODE_ELEMENT && xml->get_node_name() == "DOMLayer") {
			Ref<FlashLayer> layer = add_child<FlashLayer>(xml, &layers);
            if (layer->get_duration() > get_duration()) set_duration(layer->get_duration());
        }
    }
    return Error::OK;
}
void FlashTimeline::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    for (List<Ref<FlashLayer>>::Element *E = layers.back(); E; E = E->prev()) {
        E->get()->draw(node, time, tr, effect);
    }
}

void FlashLayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_type"), &FlashLayer::get_type);
    ClassDB::bind_method(D_METHOD("set_type", "type"), &FlashLayer::set_type);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashLayer::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashLayer::set_duration);
    ClassDB::bind_method(D_METHOD("get_frames"), &FlashLayer::get_frames);
    ClassDB::bind_method(D_METHOD("set_frames", "frames"), &FlashLayer::set_frames);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_type", "get_type");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_duration", "get_duration");
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
        name = xml->get_attribute_value("name");
    if (xml->has_attribute("color"))
        color = parse_color(xml->get_attribute_value("color"));
    if (xml->has_attribute("layerType"))
        type = xml->get_attribute_value("layerType");
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
void FlashLayer::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    if (type == "guide") return;
    float frame_time = time;// / document->get_frame_size();
    int frame_idx = static_cast<int>(ceil(frame_time)) % duration;
    frame_time = frame_idx + frame_time - (int)frame_time;
    
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

    for (List<Ref<FlashDrawing>>::Element *E = current->elements.front(); E; E = E->next()) {
        E->get()->draw(node, frame_time - current->get_index(), tr, effect);
    }
}

void FlashDrawing::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_transform"), &FlashDrawing::get_transform);
    ClassDB::bind_method(D_METHOD("set_transform", "transform"), &FlashDrawing::set_transform);

    ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM2D, "transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_transform", "get_transform");
}
void FlashDrawing::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
}

void FlashFrame::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_index"), &FlashFrame::get_index);
    ClassDB::bind_method(D_METHOD("set_index", "index"), &FlashFrame::set_index);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashFrame::get_duration);
    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &FlashFrame::set_duration);
    ClassDB::bind_method(D_METHOD("get_keymode"), &FlashFrame::get_keymode);
    ClassDB::bind_method(D_METHOD("set_keymode", "keymode"), &FlashFrame::set_keymode);
    ClassDB::bind_method(D_METHOD("get_elements"), &FlashFrame::get_elements);
    ClassDB::bind_method(D_METHOD("set_elements", "elements"), &FlashFrame::set_elements);
    ClassDB::bind_method(D_METHOD("get_tweens"), &FlashFrame::get_tweens);
    ClassDB::bind_method(D_METHOD("set_tweens", "tweens"), &FlashFrame::set_tweens);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "index", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_index", "get_index");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "duration", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_duration", "get_duration");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "keymode", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_keymode", "get_keymode");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elements", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_elements", "get_elements");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tweens", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_tweens", "get_tweens");
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
        Ref<FlashDrawing> tween = p_tweens[i];
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
    if (xml->has_attribute("index")) index = xml->get_attribute_value("index").to_int();
    if (xml->has_attribute("duration")) duration = xml->get_attribute_value("duration").to_int();
    if (xml->has_attribute("keymode")) keymode = xml->get_attribute_value("keymode");
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMFrame" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
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
    }
    return Error::OK;
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
void FlashGroup::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    List<Ref<FlashDrawing>> ms = all_members();
    for (List<Ref<FlashDrawing>>::Element *E = ms.front(); E; E = E->next()) {
        E->get()->draw(node, time, tr, effect);
    }
}

void FlashInstance::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_first_frame"), &FlashInstance::get_first_frame);
    ClassDB::bind_method(D_METHOD("set_first_frame", "first_frame"), &FlashInstance::set_first_frame);
    ClassDB::bind_method(D_METHOD("get_loop"), &FlashInstance::get_loop);
    ClassDB::bind_method(D_METHOD("set_loop", "loop"), &FlashInstance::set_loop);
    ClassDB::bind_method(D_METHOD("get_color_effect"), &FlashInstance::get_color_effect);
    ClassDB::bind_method(D_METHOD("set_color_effect", "color_effect"), &FlashInstance::set_color_effect);
    ClassDB::bind_method(D_METHOD("get_library_item_name"), &FlashInstance::get_library_item_name);
    ClassDB::bind_method(D_METHOD("set_library_item_name", "library_item_name"), &FlashInstance::set_library_item_name);
    //ClassDB::bind_method(D_METHOD("get_timeline"), &FlashInstance::get_timeline);
    //ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &FlashInstance::set_timeline);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "first_frame", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_first_frame", "get_first_frame");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "loop", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_loop", "get_loop");
    ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "color_effect", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_color_effect", "get_color_effect");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "library_item_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_library_item_name", "get_library_item_name");
    //ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "timeline", PROPERTY_HINT_RESOURCE_TYPE, "FlashTimeline", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_timeline", "get_timeline");
}
Ref<FlashTimeline> FlashInstance::get_timeline() {
    return document->load_symbol(library_item_name);
    //if (!timeline.is_valid()) 
    //    timeline = document->load_symbol(library_item_name);
    //return timeline;
}
PoolColorArray FlashInstance::get_color_effect() const {
    PoolColorArray effect;
    effect.resize(2);
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
        color_effect.mult = Color(1,1,1);
    }
}
Error FlashInstance::parse(Ref<XMLParser> xml) {
    if (xml->has_attribute("libraryItemName")) {
        library_item_name = xml->get_attribute_value("libraryItemName");
    }
    if (xml->has_attribute("firstFrame"))
        first_frame = xml->get_attribute_value("firstFrame").to_int();
    if (xml->has_attribute("loop"))
        loop = xml->get_attribute_value("loop");
    if (xml->has_attribute("centerPoint3DX"))
        center_point.x = xml->get_attribute_value("centerPoint3DX").to_float();
    if (xml->has_attribute("centerPoint3DY"))
        center_point.y = xml->get_attribute_value("centerPoint3DY").to_float();
    while (xml->read() == Error::OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMSymbolInstance" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Matrix")
            transform = parse_transform(xml);
        if (xml->get_node_name() == "Point") {
            if (xml->has_attribute("x"))
                transformation_point.x = xml->get_attribute_value("x").to_float();
            if (xml->has_attribute("y"))
                transformation_point.y = xml->get_attribute_value("y").to_float();
        }
        if (xml->get_node_name() == "Color") {
            if (xml->has_attribute("tintColor")) {
                Color tint = parse_color(xml->get_attribute_value("tintColor"));
                float amount = xml->get_attribute_value("tintMultiplier").to_float();
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
                color_effect.mult.r = xml->has_attribute("redMultiplier") ? xml->get_attribute_value("redMultiplier").to_float() : 1.0;
                color_effect.mult.g = xml->has_attribute("greenMultiplier") ? xml->get_attribute_value("greenMultiplier").to_float() : 1.0;
                color_effect.mult.b = xml->has_attribute("blueMultiplier") ? xml->get_attribute_value("blueMultiplier").to_float() : 1.0;
                color_effect.mult.a = xml->has_attribute("alphaMultiplier") ? xml->get_attribute_value("alphaMultiplier").to_float() : 1.0;
                color_effect.add.r = xml->has_attribute("greenOffset") ? xml->get_attribute_value("redOffset").to_float()/255.0 : 0.0;
                color_effect.add.g = xml->has_attribute("greenOffset") ? xml->get_attribute_value("greenOffset").to_float()/255.0 : 0.0;
                color_effect.add.b = xml->has_attribute("blueOffset") ? xml->get_attribute_value("blueOffset").to_float()/255.0 : 0.0;
                color_effect.add.a = xml->has_attribute("alphaOffset") ? xml->get_attribute_value("alphaOffset").to_float()/255.0 : 0.0;
            } else if (xml->has_attribute("alphaMultiplier")) {
                color_effect.mult.a = xml->get_attribute_value("alphaMultiplier").to_float();
            } else if (xml->has_attribute("brightness")) {
                float b = xml->get_attribute_value("brightness").to_float();
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
        }
    }
    return Error::OK;
}
void FlashInstance::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    Ref<FlashTimeline> tl = get_timeline();
    if (!tl.is_valid()) return;
    float instance_time =
        loop == "single frame"  ? first_frame :
        loop == "play once"     ? MIN(first_frame + time, tl->get_duration()) :
                                  first_frame + time;
    
    tl->draw(node, instance_time, tr * transform, effect);
    
}

void FlashBitmapInstance::_bind_methods(){
    ClassDB::bind_method(D_METHOD("get_library_item_name"), &FlashBitmapInstance::get_library_item_name);
    ClassDB::bind_method(D_METHOD("set_library_item_name", "library_item_name"), &FlashBitmapInstance::set_library_item_name);
   
    ADD_PROPERTY(PropertyInfo(Variant::INT, "library_item_name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_library_item_name", "get_library_item_name");
}
Error FlashBitmapInstance::parse(Ref<XMLParser> xml) {
    if(xml->has_attribute("libraryItemName"))
        library_item_name = xml->get_attribute_value("libraryItemName");
    while (xml->read() == OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == "DOMBitmapInstance" && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Matrix")
            transform = parse_transform(xml);
    }
    return Error::OK;
}
void FlashBitmapInstance::draw(FlashPlayer* node, float time, Transform2D tr, FlashColorEffect effect) {
    node->draw_set_transform_matrix(tr * transform);
    Ref<Texture> tex = document->load_bitmap(library_item_name);
    node->draw_texture(tex, Vector2());
}

void FlashTween::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_target"), &FlashTween::get_target);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &FlashTween::set_target);
    ClassDB::bind_method(D_METHOD("get_points"), &FlashTween::get_points);
    ClassDB::bind_method(D_METHOD("set_points", "points"), &FlashTween::set_points);
    
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_target", "get_target");
    ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "set_points", "get_points");
}
Error FlashTween::parse(Ref<XMLParser> xml) {
    String n = xml->get_node_name();
    if (xml->has_attribute("target"))
        target = xml->get_attribute_value("target");
    while (xml->read() == OK) {
        if (xml->get_node_type() == XMLParser::NODE_TEXT) continue;
        if (xml->get_node_name() == n && (xml->get_node_type() == XMLParser::NODE_ELEMENT_END || xml->is_empty()))
            return Error::OK;
        if (xml->get_node_name() == "Point") {
            Vector2 p = Vector2();
            if (xml->has_attribute("x")) p.x = xml->get_attribute_value("x").to_float();
            if (xml->has_attribute("y")) p.y = xml->get_attribute_value("y").to_float();
            points.push_back(p);
        }
    }
    return Error::OK;
}

void ResourceFormatLoaderFlashTexture::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("ftex");
}
bool ResourceFormatLoaderFlashTexture::handles_type(const String &p_type) const {
	return p_type == "StreamTexture" || p_type == "Texture";
}
String ResourceFormatLoaderFlashTexture::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "ftex")
		return "Texture";
	return "";
}
#endif