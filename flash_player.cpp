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

#include "flash_player.h"

#ifdef TOOLS_ENABLED
#include <core/engine.h>
#endif

RID FlashPlayer::normal_shader = RID();
RID FlashPlayer::metaball_shader = RID();

void FlashPlayer::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            set_process(true);
        } break;

        case NOTIFICATION_PROCESS: {
            performance_triangles_generated = 0;
            if (playing && active_symbol.is_valid()) {
                advance(get_process_delta_time(), false, true);
            }
        } break;

        case NOTIFICATION_DRAW: {
            switch (render_mode) {
                case RENDER_NORMAL: {
                    _draw_normal();
                } break;
                case RENDER_METABALL: {
                    _draw_metaball();
                } break;
            }
        } break;

        case NOTIFICATION_VISIBILITY_CHANGED: {
            performance_triangles_drawn = 0;
            performance_triangles_generated = 0;
        } break;
    }
};

void FlashPlayer::_generate_normal_shader() const {
    if (normal_shader.is_valid()) return;
    normal_shader = VisualServer::get_singleton()->shader_create();
    VisualServer::get_singleton()->shader_set_code(normal_shader,
        "shader_type canvas_item;\n"

        "uniform sampler2DArray ATLAS;\n"
        "uniform sampler2D CLIPPING_TEXTURE;\n"
        "uniform vec2 ATLAS_SIZE;\n"
        "uniform bool OVERLAY_ENABLED;\n"
        "uniform sampler2D OVERLAY_TEXTURE;\n"
        "varying float CLIPPING_SIZE;\n"
        "varying float CLIPPING_IDX[4];"
        "varying vec4 CLIPPING_UV[4];\n"
        "varying float TEX_IDX;\n"

        "void vertex() {\n"
        "   float clipping_size_with_tex_idx = 0.0;\n"
        "   float clipping_id = 0.0;\n"
        "   UV.x = 2.0 * modf(UV.x, clipping_id);\n"
        "   UV.y = 2.0 * modf(UV.y, clipping_size_with_tex_idx);\n"
        "   TEX_IDX = float(int(clipping_size_with_tex_idx) & 255);\n"
        "   float clipping_size = float(int(clipping_size_with_tex_idx) >> 8);\n"
        "   CLIPPING_SIZE = min(clipping_size, 4.0);\n"
        "   for (int i=0; i<int(CLIPPING_SIZE); i++) {"
        "       int dcx = int(clipping_id*4.0) % 32;\n"
        "       int dcy = int(clipping_id*4.0) / 32;\n"
        "       vec4 tr_xy = texelFetch(CLIPPING_TEXTURE, ivec2(dcx, dcy), 0);\n"
        "       vec4 tr_origin = texelFetch(CLIPPING_TEXTURE, ivec2(dcx+1, dcy), 0);\n"
        "       vec4 tex_region = texelFetch(CLIPPING_TEXTURE, ivec2(dcx+2, dcy), 0);\n"
        "       vec2 tex_pos = tex_region.xy;\n"
        "       vec2 tex_size = tex_region.zw;\n"

        "       mat4 tr = mat4(\n"
        "           vec4(tr_xy.r, tr_xy.g, 0.0, 0.0),\n"
        "           vec4(tr_xy.b, tr_xy.a, 0.0, 0.0),\n"
        "           vec4(0.0, 0.0, 1.0, 0.0),\n"
        "           vec4(tr_origin.r, tr_origin.g, 0.0, 1.0)\n"
        "       );\n"
        "       mat4 local = tr * WORLD_MATRIX * EXTRA_MATRIX;\n"
        "       vec2 clipping_pos = (local * vec4(VERTEX, 0.0 ,1.0)).xy;\n"
        "       CLIPPING_UV[i].xy = clipping_pos / tex_size;\n"
        "       CLIPPING_UV[i].zw = (clipping_pos + tex_pos)/ATLAS_SIZE;\n"
        "       CLIPPING_IDX[i] = tr_origin.b;\n"
        "   }\n"
        "}\n"

        "void fragment() {\n"
        "   float masked = 1.0;\n"
        "   if (int(CLIPPING_SIZE) > 0) masked = 0.0;\n"
        "   for (int i=0; i<int(CLIPPING_SIZE); i++) {\n"
        "       if (CLIPPING_UV[i].x >= 0.0 && CLIPPING_UV[i].x < 1.0 && CLIPPING_UV[i].y >= 0.0 && CLIPPING_UV[i].y < 1.0) {\n"
        "           vec4 mask = textureLod(ATLAS, vec3(CLIPPING_UV[i].zw, CLIPPING_IDX[i]), 0.0);\n"
        "           if (mask.a >= 1.0) {\n"
        "               masked = 1.0;\n"
        "               break;\n"
        "           }\n"
        "           masked = max(masked, mask.a);\n"
        "       }\n"
        "   }\n"
        "   if (masked > 0.0) {\n"
        "       vec4 add;\n"
        "       vec4 c = texture(ATLAS, vec3(UV, TEX_IDX));\n"
        "       vec4 mult = 2.0*modf(COLOR, add);\n"
        "       COLOR = clamp(abs(c * mult) + add / 255.0, vec4(0.0), vec4(1.0));\n"
        "       if (OVERLAY_ENABLED) {\n"
        "           COLOR.rgb = texture(OVERLAY_TEXTURE, SCREEN_UV).rgb;\n"
        "       }"
        "       if (c.a <= 0.0) {\n"
        "           COLOR.a = 0.0;\n"
        "       } else {\n"
        "           COLOR.a = min(COLOR.a, masked);\n"
        "       }\n"
        "   } else {\n"
        "       COLOR = vec4(0.0);\n"
        "   }\n"
        "}\n"
    );
}

void FlashPlayer::_generate_metaball_shader() const {
    if (metaball_shader.is_valid()) return;
    metaball_shader = VisualServer::get_singleton()->shader_create();
    VisualServer::get_singleton()->shader_set_code(metaball_shader,
        "shader_type canvas_item;\n"
        "uniform bool OVERLAY_ENABLED;\n"
        "uniform sampler2D OVERLAY_TEXTURE;\n"
        "uniform float THRESHOLD : hint_range(0.0, 2.0) = 0.3;\n"
        "uniform bool DEBUG_ENABLED;\n"
        "uniform int CIRCLES_COUNT;\n"
        "uniform vec4 CIRCLE_0;\n"
        "uniform vec4 CIRCLE_1;\n"
        "uniform vec4 CIRCLE_2;\n"
        "uniform vec4 CIRCLE_3;\n"
        "uniform vec4 CIRCLE_4;\n"
        "uniform vec4 CIRCLE_5;\n"
        "uniform vec4 CIRCLE_6;\n"
        "uniform vec4 CIRCLE_7;\n"
        "uniform vec4 CIRCLE_8;\n"
        "uniform vec4 CIRCLE_9;\n"
        "uniform vec4 CIRCLE_10;\n"
        "uniform vec4 CIRCLE_11;\n"
        "uniform vec4 CIRCLE_12;\n"
        "uniform vec4 CIRCLE_13;\n"
        "uniform vec4 CIRCLE_14;\n"
        "uniform vec4 CIRCLE_15;\n"

        "void fragment() {\n"
        "   vec2 uv = UV;\n"
        "   float v = 0.0;\n"
        "   float debug = 0.0;\n"
        "   vec4 circles[16] = vec4[](\n"
        "       CIRCLE_0, CIRCLE_1, CIRCLE_2, CIRCLE_3,\n"
        "       CIRCLE_4, CIRCLE_5, CIRCLE_6, CIRCLE_7,\n"
        "       CIRCLE_8, CIRCLE_9, CIRCLE_10, CIRCLE_11,\n"
        "       CIRCLE_12, CIRCLE_13, CIRCLE_14, CIRCLE_15\n"
        "   );\n"
        "   for (int i = 0; i < CIRCLES_COUNT; i++) {\n"
        "      vec2 center = circles[i].xy;\n"
        "      float r = circles[i].z;\n"
        "      float overweight = circles[i].w;\n"
        "      vec2 dir = uv - center;\n"
        // "      float d = dot(dir, dir);\n"
        "      float d = dot(dir, dir);\n"
        "      v += overweight * r / (d + 0.0000001);\n"
        "      if (d < r + 0.0002 && d > r - 0.0002) {\n"
		"           debug = max(debug, 1.0);\n"
		"      }\n"
        "}\n"
        // "v = 0.5 + log(v);\n"
        // "v = v / float(CIRCLES_COUNT);\n"
        // // Пример: 3 круга, их центры и радиусы можно передавать через uniform
        // vec2 centers[3] = vec2[](vec2(0.5 + 0.3*sin(TIME),0.5), vec2(
        //     0.5 + 0.35*sin(1.25*TIME),
        //     0.5 - 0.35*cos(1.25*TIME)
        // ), vec2(0.7,0.6 + 0.3*sin(0.75*TIME)));
        // float radii[3] = float[](0.1, 0.15, 0.25);
        // for (int i = 0; i < 3; i++) {
            
        //     vec2 rad = uv - centers[i];
        //     float d = dot(rad, rad);
        //     v += radii[i]*radii[i] / (d + 0.0001);
        // }
            
        "   float edge = 0.01;\n"
        "   float alpha = smoothstep(THRESHOLD - edge, THRESHOLD + edge, v);\n"
        "   if (DEBUG_ENABLED && debug > 0.0) {\n"
        "       COLOR = vec4(0.9, 0.1, 0.1, 1);\n"
        
        "   } else if (alpha > 0.01) {\n"
        // "   if (alpha > 0.01) {\n"
        "       if (OVERLAY_ENABLED) {\n"
        "           COLOR.rgb = texture(OVERLAY_TEXTURE, SCREEN_UV).rgb;\n"
        "       }"
        "       COLOR.a = alpha;\n"
        "   } else {\n"
        "       discard;\n"    
        // "       COLOR = vec4(1.0, 1.0, 1.0, 0.2);\n"
        "   }\n"
        "}\n"

    );
}

void FlashPlayer::_draw_normal() {
    if (active_symbol.is_valid() && points.size() > 0 && resource.is_valid()) {
        update_clipping_data();
        VisualServer::get_singleton()->mesh_clear(mesh);
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = points;
        arrays[Mesh::ARRAY_INDEX] = indices;
        arrays[Mesh::ARRAY_COLOR] = colors;
        arrays[Mesh::ARRAY_TEX_UV] = uvs;
        VisualServer::get_singleton()->mesh_add_surface_from_arrays(
            mesh,
            VisualServer::PRIMITIVE_TRIANGLES,
            arrays, Array(),
            VisualServer::ARRAY_FLAG_USE_2D_VERTICES
        );
        VisualServer::get_singleton()->canvas_item_add_mesh(get_canvas_item(), mesh);
        performance_triangles_drawn = indices.size() / 3;
    }
}

void FlashPlayer::_draw_metaball() {
    if (metaball_cache.size() == 0) return;
    if (metaball_rect.size.x <= 0.0 || metaball_rect.size.y <= 0.0) {
        return; // No metaball to draw
    }
    Vector2 size;
    float factor;
    metaball_rect = metaball_rect.expand(metaball_rect.position - Vector2(150., 150.));
    metaball_rect = metaball_rect.expand(metaball_rect.position + metaball_rect.size + Vector2(150., 150.));
    if (metaball_rect.size.x > metaball_rect.size.y) {
        size.y = 1.0;
        factor = 1.0 / metaball_rect.size.y;
        size.x = metaball_rect.size.x / metaball_rect.size.y;
    } else {
        size.x = 1.0;
        factor = 1.0 / metaball_rect.size.x;
        size.y = metaball_rect.size.y / metaball_rect.size.x;
    }
    
    points.push_back(metaball_rect.position);
    points.push_back(metaball_rect.position + Vector2(metaball_rect.size.x, 0.0));
    points.push_back(metaball_rect.position + metaball_rect.size);
    points.push_back(metaball_rect.position + Vector2(0.0, metaball_rect.size.y));
    uvs.push_back(Vector2(0.0, 0.0));
    uvs.push_back(Vector2(size.x, 0.0));
    uvs.push_back(Vector2(size.x, size.y));
    uvs.push_back(Vector2(0.0, size.y));
    colors.push_back(metaball_color);
    colors.push_back(metaball_color);
    colors.push_back(metaball_color);
    colors.push_back(metaball_color);
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // draw_polygon(points, colors, uvs);
    // draw_rect(metaball_rect, Color(1.0, 1.0, 1.0, 1.0), true);

    VisualServer::get_singleton()->mesh_clear(mesh);
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = points;
    arrays[Mesh::ARRAY_INDEX] = indices;
    arrays[Mesh::ARRAY_COLOR] = colors;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    VisualServer::get_singleton()->mesh_add_surface_from_arrays(
        mesh,
        VisualServer::PRIMITIVE_TRIANGLES,
        arrays, Array(),
        VisualServer::ARRAY_FLAG_USE_2D_VERTICES
    );
    VisualServer::get_singleton()->canvas_item_add_mesh(get_canvas_item(), mesh);

    VisualServer::get_singleton()->material_set_param(flash_material, "CIRCLES_COUNT", metaball_cache.size());

    for (int i=0; i<metaball_cache.size(); i++){
        Vector2 ipos = Vector2(
            (metaball_cache[i].x - metaball_rect.position.x) * factor,
            (metaball_cache[i].y - metaball_rect.position.y) * factor
        );
        float radius = metaball_cache[i].z * factor;
        float balancing_factor = 1.0;
        if (metaball_weight_balancing == WEIGHT_BALANCE_LINEAR) {
            balancing_factor = 1.25;
        } else if (metaball_weight_balancing == WEIGHT_BALANCE_EXPONENTIAL) {
            balancing_factor = 2.0;
        }
        float irad = radius * balancing_factor;
        float iirad = irad * irad;
        float overweight = 0.0;
        float area = Math_PI * iirad;
        if (metaball_weight_balancing != WEIGHT_BALANCE_NONE) {
            for (int j=0; j<metaball_cache.size(); j++) {
                Vector2 jpos = Vector2(
                    (metaball_cache[j].x - metaball_rect.position.x) * factor,
                    (metaball_cache[j].y - metaball_rect.position.y) * factor
                );
                float jrad = metaball_cache[j].z * factor * balancing_factor;
                float jjrad = jrad * jrad;
				float dist = (ipos - jpos).length();
                float dd = dist * dist;
                if (dist > irad + jrad) {
                    continue;
                }
				if (dist <= abs(irad - jrad)) {
					float mrad = fmin(iirad, jjrad);
					overweight += Math_PI * mrad;
					continue;
                }
				float alpha = 2.0 * acos((dd + iirad - jjrad) / (2.0*dist*irad));
				float beta = 2.0 * acos((dd + jjrad - iirad) / (2.0*dist*jrad));
				float s1 = 0.5 * iirad * (alpha - sin(alpha));
				float s2 = 0.5 * jjrad * (beta  - sin(beta));
				overweight += s1 + s2;
            }
        } else {
			area = 1.0;
        }
        overweight = area / (area + overweight);
        if (metaball_weight_balancing == WEIGHT_BALANCE_EXPONENTIAL) {
            overweight = sqrt(overweight);
        }

        Color circle = Color(ipos.x, ipos.y, radius * radius, overweight);

        VisualServer::get_singleton()->material_set_param(flash_material, "CIRCLE_" + itos(i), circle);
    }
    VisualServer::get_singleton()->material_set_param(flash_material, "THRESHOLD", metaball_threshold);
    VisualServer::get_singleton()->material_set_param(flash_material, "DEBUG_ENABLED", metaball_debug);
    indices.resize(0);
    points.resize(0);
    uvs.resize(0);
    colors.resize(0);
}
void FlashPlayer::override_frame(String p_symbol, Variant p_value) {
    //ERR_FAIL_COND_MSG(resource.is_null(), "Can't override symbol without resource");
    if (resource.is_null()) return;
    Ref<FlashTimeline> symbol = resource->get_symbols().get(p_symbol, Ref<FlashTimeline>());
    if (symbol.is_null()) return;
    if (symbol->get_variation_idx() < 0) return;
    if (p_value.get_type() == Variant::NIL) {
        frame_overrides.set(symbol->get_variation_idx(), -1);
        queue_process();
    } else if (p_value.get_type() == Variant::REAL || p_value.get_type() == Variant::INT) {
        frame_overrides.set(symbol->get_variation_idx(), p_value);
        queue_process();
    }
}
void FlashPlayer::set_variant(String variant, Variant value) {
    if (value == Variant() || value == "[default]") {
        if(active_variants.has(variant)) active_variants.erase(variant);
    } else {
        active_variants[variant] = value;
    }

    if (!resource.is_valid()) return;
    Dictionary variants = resource->get_variants();
    if (variants.has(variant)) {
        Dictionary symbols_by_variant = variants[variant];
        if (symbols_by_variant.has(value)) {
            if (value == "[default]") {
                for (int i=0; i<symbols_by_variant.size(); i++) {
                    Ref<FlashTimeline> symbol = resource->get_symbols().get(symbols_by_variant.get_key_at_index(i), Variant());
                    if (symbol.is_null() || symbol->get_variation_idx() < 0) continue;
                    frame_overrides.set(symbol->get_variation_idx(), -1);
                }
            } else {
                Dictionary frames_by_symbol = symbols_by_variant[value];
                for (int i=0; i<frames_by_symbol.size(); i++) {
                    String token = frames_by_symbol.get_key_at_index(i);
                    Ref<FlashTimeline> symbol = resource->get_symbols().get(token, Variant());
                    if (symbol.is_null() || symbol->get_variation_idx() < 0) continue;
                    int frame = frames_by_symbol.get_value_at_index(i);
                    frame_overrides.set(symbol->get_variation_idx(), frame);
                }
            }
        }
    }
    queue_process();
}
String FlashPlayer::get_variant(String variant) const {
    return active_variants.has(variant) ? active_variants[variant] : "[default]";
}

void FlashPlayer::set_clip(String clip, Variant value) {
    if (!resource.is_valid()) return;
    if (value == Variant() || value == "[default]") {
        if(clips_state.has(clip)) clips_state.erase(clip);
        if(active_clips.has(clip)) active_clips.erase(clip);
    } else {
        String *track_clip = active_clips.getptr(clip);
        if (track_clip == NULL || *track_clip == value) return;
        active_clips[clip] = value;
        Array timelines = resource->get_symbols().values();
        for (int i=0; i<timelines.size(); i++) {
            Ref<FlashTimeline> tl = timelines[i];
            if (clip != tl->get_clips_header()) continue;
            Vector2 clip_data = tl->get_clips()[value];
            clips_state[clip] = Vector3(clip_data.x, clip_data.y, 0.0);
            break;
        }
    }
    tracks_dirty = true;
    queue_process();
}

String FlashPlayer::get_clip(String clip) const {
    return active_clips.has(clip) ? active_clips[clip] : "[default]";
}

float FlashPlayer::get_symbol_frame(FlashTimeline* p_symbol, float p_default) {
    if (p_symbol == NULL) {
        return p_default;
    }

    Vector3 *clip = clips_state.getptr(p_symbol->get_clips_header());
    if (clip != NULL) {
        return clip->x + clip->z;        
    }

    if (p_symbol->get_variation_idx() < 0) {
        return p_default;
    }

    float frame = frame_overrides[p_symbol->get_variation_idx()];
    return frame < 0 ? p_default : frame;
}

Dictionary FlashPlayer::get_variants() const {
    if (!resource.is_valid()) return Dictionary();
    return resource->get_variants();
}

bool FlashPlayer::_set(const StringName &p_name, const Variant &p_value) {
    String n = p_name;
    if (n.begins_with("variants/")) {
        String variant = n.substr(strlen("variants/"));
        set_variant(variant, p_value);
        return true;
    }
    if (n.begins_with("clips/")) {
        String clip = n.substr(strlen("clips/"));
        set_clip(clip, p_value);
        return true;
    }
    return false;
}

bool FlashPlayer::_get(const StringName &p_name, Variant &r_ret) const {
    String n = p_name;
    if (n.begins_with("variants/")) {
        String variant = n.substr(strlen("variants/"));
        r_ret = get_variant(variant);
        return true;
    } else if (n.begins_with("clips/")) {
        String clip = n.substr(strlen("clips/"));
        r_ret = get_clip(clip);
        return true;
    } else if (p_name == "performance/triangles_drawn") {
		r_ret = performance_triangles_drawn;
        return true;
	} else if (p_name == "performance/triangles_generated") {
		r_ret = performance_triangles_generated;
        return true;
	}
    return false;
}

void FlashPlayer::_get_property_list(List<PropertyInfo> *p_list) const {
    if (!resource.is_valid()) return;
    Dictionary variants = resource->get_variants();
    for (int i=0; i<variants.size(); i++) {
        String key = variants.get_key_at_index(i);
        Dictionary options = variants[key];
        String options_string = "[default]";
        for (int j=0; j<options.size(); j++) {
            String option = options.get_key_at_index(j);
            options_string += "," + option;
        }
        p_list->push_back(PropertyInfo(Variant::STRING, "variants/" + key, PROPERTY_HINT_ENUM, options_string));
    }
    HashMap<String, Vector<String>> clips;
    Array timelines = resource->get_symbols().values();
    for (int i=0; i<timelines.size(); i++) {
        Ref<FlashTimeline> tl = timelines[i];
        String clips_header = tl->get_clips_header();
        if (clips_header == String()) continue;
        Array clip_names = tl->get_clips().keys();
        for (int i=0; i<clip_names.size(); i++) {
            String clip_name = clip_names[i];
            if (clips[clips_header].find(clip_name) < 0) {
                clips[clips_header].push_back(clip_name);
            }
        }
    }
    List<String> clip_keys;
    clips.get_key_list(&clip_keys);
    for (List<String>::Element *E = clip_keys.front(); E; E = E->next()) {
        String clips_key = E->get();
        Vector<String> timeline_clips = clips[clips_key];
        timeline_clips.insert(0, "[default]");
        p_list->push_back(PropertyInfo(Variant::STRING, "clips/" + clips_key, PROPERTY_HINT_ENUM, String(",").join(timeline_clips)));
    }
}

PoolStringArray FlashPlayer::get_clips_tracks() const {
    PoolStringArray result;
    Dictionary unique;
    if (!resource.is_valid()) return result;
    Array timelines = resource->get_symbols().values();
    for (int i=0; i<timelines.size(); i++) {
        Ref<FlashTimeline> tl = timelines[i];
        String clips_track = tl->get_clips_header();
        if (clips_track == String()) continue;
        if (!unique.has(clips_track)) {
            unique[clips_track] = true;
            result.push_back(clips_track);
        }
    }
    return result;
}

PoolStringArray FlashPlayer::get_clips_for_track(const String &track) const {
    PoolStringArray result;
    Dictionary cache;
    if (!resource.is_valid()) return result;
    Array timelines = resource->get_symbols().values();
    for (int i=0; i<timelines.size(); i++) {
        Ref<FlashTimeline> tl = timelines[i];
        String clips_track = tl->get_clips_header();
        if (clips_track != track) continue;
        Array clip_names = tl->get_clips().keys();
        for (int i=0; i<clip_names.size(); i++) {
            String clip_name = clip_names[i];
            if (!cache.has(clip_name)) {
                cache[clip_name] = true;
                result.push_back(clip_name);
            }
        }
    }
    return result;
}

float FlashPlayer::get_clip_duration(const String &header, const String &clip) const {
    return 0.0;
}

void FlashPlayer::set_render_mode(RenderMode p_mode) {
    if (render_mode == p_mode) return;
    render_mode = p_mode;
    if (render_mode == RENDER_NORMAL) {
        VisualServer::get_singleton()->material_set_shader(flash_material, normal_shader);
        if (resource.is_valid()) {
            VisualServer::get_singleton()->material_set_param(flash_material, "ATLAS_SIZE", resource->get_atlas_size());
            VisualServer::get_singleton()->material_set_param(flash_material, "ATLAS", resource->get_atlas());
        }
        VisualServer::get_singleton()->material_set_param(flash_material, "CLIPPING_TEXTURE", clipping_texture);
        VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_ENABLED", overlay_texture.is_valid());
        if (overlay_texture.is_valid()) {
            VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_TEXTURE", overlay_texture);
        } else {
            VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_TEXTURE", RID());
        }
    } else if (render_mode == RENDER_METABALL) {
        VisualServer::get_singleton()->material_set_shader(flash_material, metaball_shader);
        VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_ENABLED", overlay_texture.is_valid());
        if (overlay_texture.is_valid()) {
            VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_TEXTURE", overlay_texture);
        } else {
            VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_TEXTURE", RID());
        }
    }
}

void FlashPlayer::set_metaball_threshold(float p_threshold) {
    if (p_threshold < 0.0 || p_threshold > 2.0) {
        ERR_FAIL_MSG("Metaballs threshold must be in range [0.0, 3.0]");
    }
    if (metaball_threshold == p_threshold) return;
    
    metaball_threshold = p_threshold;
    update();
}

void FlashPlayer::set_metaball_debug(bool p_debug) {
    if (metaball_debug == p_debug) return;
    metaball_debug = p_debug;
    update();
}

void FlashPlayer::set_metaball_weight_balancing(WeightBalancing p_balancing) {
    if (metaball_weight_balancing == p_balancing) return;
    metaball_weight_balancing = p_balancing;
    update();
}

void FlashPlayer::set_metaball_color(const Color &p_color) {
    if (metaball_color == p_color) return;
    metaball_color = p_color;
    update();
}

void FlashPlayer::_validate_property(PropertyInfo &prop) const {
    if (prop.name == "active_symbol"){
        String symbols_hint = "[document]";
        if (resource.is_valid()) {
            Array symbols = resource->get_symbols().values();
            for (int i=0; i<symbols.size(); i++){
                Ref<FlashTimeline> symbol = symbols[i];
                if (symbol->get_local_path().find("/") >= 0) continue;
                symbols_hint += "," + symbol->get_token();
            }
        }
        prop.hint_string = symbols_hint;
    }

    if (prop.name == "active_clip") {
        String clips_hint = "[full]";
        if (active_symbol.is_valid()) {
            Array clips = active_symbol->get_clips().keys();
            if (clips.size() > 0) {
                prop.usage = PROPERTY_USAGE_DEFAULT;
            } else {
                prop.usage = PROPERTY_USAGE_NOEDITOR;
                return;
            }

            clips.sort_custom((FlashPlayer*)this, "_sort_clips");
            for (int i=0; i<clips.size(); i++){
                String clip = clips[i];
                clips_hint += "," + clip;
            }
            prop.hint_string = clips_hint;
        } else {
            prop.usage = PROPERTY_USAGE_NOEDITOR;
        }
    }

    if (prop.name == "material" || prop.name == "use_parent_material") {
        prop.usage = PROPERTY_USAGE_NOEDITOR|PROPERTY_USAGE_RESOURCE_NOT_PERSISTENT;
    }
    if (prop.name.begins_with("metaball")) {
        if (render_mode != RENDER_METABALL) {
            prop.usage = PROPERTY_USAGE_NOEDITOR;
        } else {
            prop.usage = PROPERTY_USAGE_DEFAULT;
        }
    }
}
bool FlashPlayer::_sort_clips(Variant a, Variant b) const {
    if (!active_symbol.is_valid()) return false;
    Vector2 da = active_symbol->get_clips()[a];
    Vector2 db = active_symbol->get_clips()[b];
    return da.x < db.x;
}

void FlashPlayer::set_resource(const Ref<FlashDocument> &doc) {
    if (doc != resource) active_symbol_name = "[document]";
    resource = doc;
    frame = 0;
    processed_frame = -1;
    playback_start = 0;
    playback_end = 0;
    frame_overrides.clear();
    active_variants.clear();
    if (resource.is_valid()) {
        frame_overrides.resize(resource->get_variated_symbols_count());
        for (int i=0; i<frame_overrides.size(); i++) { frame_overrides.set(i, -1); }
        active_symbol = resource->get_main_timeline();
        if (active_symbol.is_valid())
            playback_end = active_symbol->get_duration();
        if (render_mode == RENDER_NORMAL) {
            VisualServer::get_singleton()->material_set_param(flash_material, "ATLAS_SIZE", resource->get_atlas_size());
            VisualServer::get_singleton()->material_set_param(flash_material, "ATLAS", resource->get_atlas());
        }
    } else {
        frame_overrides.resize(0);
    }
    queue_process();
    _change_notify();
    emit_signal("resource_changed");
}

Ref<FlashDocument> FlashPlayer::get_resource() const {
    return resource;
}

void FlashPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("override_frame", "symbol", "frame"), &FlashPlayer::override_frame);
    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &FlashPlayer::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &FlashPlayer::is_playing);
    ClassDB::bind_method(D_METHOD("set_loop", "loop"), &FlashPlayer::set_loop);
    ClassDB::bind_method(D_METHOD("is_loop"), &FlashPlayer::is_loop);
    ClassDB::bind_method(D_METHOD("set_frame_rate", "frame_rate"), &FlashPlayer::set_frame_rate);
    ClassDB::bind_method(D_METHOD("get_frame_rate"), &FlashPlayer::get_frame_rate);
    ClassDB::bind_method(D_METHOD("set_frame", "frame"), &FlashPlayer::set_frame);
    ClassDB::bind_method(D_METHOD("get_frame"), &FlashPlayer::get_frame);
    ClassDB::bind_method(D_METHOD("set_variant", "variant", "value"), &FlashPlayer::set_variant);
    ClassDB::bind_method(D_METHOD("get_variant", "variant"), &FlashPlayer::get_variant);
    ClassDB::bind_method(D_METHOD("get_variants"), &FlashPlayer::get_variants);
    ClassDB::bind_method(D_METHOD("set_resource", "resource"), &FlashPlayer::set_resource);
    ClassDB::bind_method(D_METHOD("get_resource"), &FlashPlayer::get_resource);
    ClassDB::bind_method(D_METHOD("get_duration"), &FlashPlayer::get_duration, DEFVAL(String()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("set_active_symbol", "active_symbol"), &FlashPlayer::set_active_symbol);
    ClassDB::bind_method(D_METHOD("get_active_symbol"), &FlashPlayer::get_active_symbol);
    ClassDB::bind_method(D_METHOD("set_active_clip", "active_clip"), &FlashPlayer::set_active_clip);
    ClassDB::bind_method(D_METHOD("get_active_clip"), &FlashPlayer::get_active_clip);
    ClassDB::bind_method(D_METHOD("get_overlay_texture"), &FlashPlayer::get_overlay_texture);
    ClassDB::bind_method(D_METHOD("set_overlay_texture", "texture"), &FlashPlayer::set_overlay_texture);
    ClassDB::bind_method(D_METHOD("get_render_mode"), &FlashPlayer::get_render_mode);
    ClassDB::bind_method(D_METHOD("set_render_mode", "mode"), &FlashPlayer::set_render_mode);
    ClassDB::bind_method(D_METHOD("get_metaball_threshold"), &FlashPlayer::get_metaball_threshold);
    ClassDB::bind_method(D_METHOD("set_metaball_threshold", "threshold"), &FlashPlayer::set_metaball_threshold);
    ClassDB::bind_method(D_METHOD("is_metaball_debug"), &FlashPlayer::is_metaball_debug);
    ClassDB::bind_method(D_METHOD("set_metaball_debug", "debug"), &FlashPlayer::set_metaball_debug);
    ClassDB::bind_method(D_METHOD("set_metaball_weight_balancing", "balancing"), &FlashPlayer::set_metaball_weight_balancing);
    ClassDB::bind_method(D_METHOD("get_metaball_weight_balancing"), &FlashPlayer::get_metaball_weight_balancing);
    ClassDB::bind_method(D_METHOD("set_metaball_color", "color"), &FlashPlayer::set_metaball_color);
    ClassDB::bind_method(D_METHOD("get_metaball_color"), &FlashPlayer::get_metaball_color);
    

    ClassDB::bind_method(D_METHOD("_animation_process"), &FlashPlayer::_animation_process);

    ClassDB::bind_method(D_METHOD("advance_clip_for_track", "p_track", "p_clip", "p_time", "p_seek"), &FlashPlayer::advance_clip_for_track_wrapper);
    ClassDB::bind_method(D_METHOD("advance", "p_time", "p_seek", "advance_all_frames"), &FlashPlayer::advance);

    ClassDB::bind_method(D_METHOD("_sort_clips"), &FlashPlayer::_sort_clips);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing", PROPERTY_HINT_NONE, ""), "set_playing", "is_playing");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop", PROPERTY_HINT_NONE, ""), "set_loop", "is_loop");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "frame_rate", PROPERTY_HINT_NONE, ""), "set_frame_rate", "get_frame_rate");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "resource", PROPERTY_HINT_RESOURCE_TYPE, "FlashDocument"), "set_resource", "get_resource");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_symbol", PROPERTY_HINT_ENUM, ""), "set_active_symbol", "get_active_symbol");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_clip", PROPERTY_HINT_ENUM, ""), "set_active_clip", "get_active_clip");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "replace_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_overlay_texture", "get_overlay_texture");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "render_mode", PROPERTY_HINT_ENUM, "Normal,Metaball"), "set_render_mode", "get_render_mode");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "metaball/color", PROPERTY_HINT_COLOR_NO_ALPHA), "set_metaball_color", "get_metaball_color");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "metaball/balancing", PROPERTY_HINT_ENUM, "None,Linear,Exponential"), "set_metaball_weight_balancing", "get_metaball_weight_balancing");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "metaball/threshold", PROPERTY_HINT_RANGE, "0.1,3.0,0.05"), "set_metaball_threshold", "get_metaball_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "metaball/debug", PROPERTY_HINT_NONE, ""), "set_metaball_debug", "is_metaball_debug");

    ADD_SIGNAL(MethodInfo("resource_changed"));
    ADD_SIGNAL(MethodInfo("animation_completed"));
    ADD_SIGNAL(MethodInfo("animation_event", PropertyInfo(Variant::STRING, "name")));

    ADD_SIGNAL(MethodInfo("advance_called", PropertyInfo(Variant::REAL, "p_time"), PropertyInfo(Variant::BOOL, "p_seek"), PropertyInfo(Variant::BOOL, "advance_all_frames")));
    ADD_SIGNAL(MethodInfo("advance_clip_for_track_called", PropertyInfo(Variant::STRING, "p_track"), PropertyInfo(Variant::STRING, "p_clip"), PropertyInfo(Variant::REAL, "p_time"), PropertyInfo(Variant::BOOL, "p_seek")));
    ADD_SIGNAL(MethodInfo("set_active_symbol_called", PropertyInfo(Variant::STRING, "p_value")));
    ADD_SIGNAL(MethodInfo("set_active_clip_called", PropertyInfo(Variant::STRING, "p_value")));

    // compatiblity
    ClassDB::bind_method(D_METHOD("set_active_label", "active_label"), &FlashPlayer::set_active_clip);
    ClassDB::bind_method(D_METHOD("get_active_label"), &FlashPlayer::get_active_clip);
    ClassDB::bind_method(D_METHOD("set_active_timeline", "active_timeline"), &FlashPlayer::set_active_symbol);
    ClassDB::bind_method(D_METHOD("get_active_timeline"), &FlashPlayer::get_active_symbol);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_label", PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_NOEDITOR), "set_active_label", "get_active_label");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "active_timeline", PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_NOEDITOR), "set_active_timeline", "get_active_timeline");

    BIND_ENUM_CONSTANT(RENDER_NORMAL);
    BIND_ENUM_CONSTANT(RENDER_METABALL);
    BIND_ENUM_CONSTANT(WEIGHT_BALANCE_NONE);
    BIND_ENUM_CONSTANT(WEIGHT_BALANCE_LINEAR);
    BIND_ENUM_CONSTANT(WEIGHT_BALANCE_EXPONENTIAL);

}

float FlashPlayer::get_duration(String p_symbol, String p_clip) {
    if (!resource.is_valid()) return 0;
    return resource->get_duration(p_symbol, p_clip);
}

void FlashPlayer::set_active_symbol(String p_value) {    
    emit_signal("set_active_symbol_called", p_value);
    if (p_value == "[document]") p_value = "";
    if (active_symbol_name == p_value) return;
    active_symbol_name = p_value;
    active_clip = "";
    frame = 0;
    processed_frame = -1;
    playback_start = 0;
    if (resource.is_valid() && resource->get_symbols().has(active_symbol_name)) {
        active_symbol = resource->get_symbols()[active_symbol_name];
    } else if (resource.is_valid()){
        active_symbol = resource->get_main_timeline();
    } else {
        active_symbol_name = "";
    }

    if (active_symbol.is_valid()) {
        playback_end = active_symbol->get_duration();
    } else {
        playback_end = 0;
    }
    queue_process();
    _change_notify();
}
String FlashPlayer::get_active_symbol() const {
    return active_symbol_name == String() ? "[document]" : active_symbol_name;
}

void FlashPlayer::set_active_clip(String p_value) {
    emit_signal("set_active_clip_called", p_value);
    if (p_value == "[full]") p_value = "";
    if (active_clip == p_value) return;
    active_clip = p_value;
    if (active_symbol.is_valid()) {
        Dictionary clips = active_symbol->get_clips();
        if (clips.has(p_value)) {
            Vector2 clip = clips[p_value];
            playback_start = clip.x;
            playback_end = clip.y;

        } else {
            playback_start = 0;
            playback_end = active_symbol->get_duration();
        }
        frame = playback_start;
    } else {
        active_clip = "";
    }
    queue_process();
    update();
}

String FlashPlayer::get_active_clip() const {
    return active_clip == String() ? "[full]" : active_clip;
}

Ref<Texture> FlashPlayer::get_overlay_texture() const {
    return overlay_texture;
}

void FlashPlayer::set_overlay_texture(const Ref<Texture> &p_texture) {
    overlay_texture = p_texture;
    VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_ENABLED", overlay_texture.is_valid());
    VisualServer::get_singleton()->material_set_param(flash_material, "OVERLAY_TEXTURE", overlay_texture);
}

PoolStringArray FlashPlayer::get_symbols() const {
    PoolStringArray result;
    if (!resource.is_valid()) {
        return result;
    }
    Array symbols = resource->get_symbols().values();
    for (int i=0; i<symbols.size(); i++){
        Ref<FlashTimeline> symbol = symbols[i];
        if (symbol->get_local_path().find("/") >= 0) continue;
        result.push_back(symbol->get_token());
    }
    return result;
}

PoolStringArray FlashPlayer::get_clips(String p_symbol) const {
    PoolStringArray result;
    if (!resource.is_valid()) return result;
    Ref<FlashTimeline> symbol;
    if (p_symbol == String()) {
        symbol = active_symbol;
    } else {
        Array symbols = resource->get_symbols().values();
        for (int i=0; i<symbols.size(); i++){
            Ref<FlashTimeline> s = symbols[i];
            if (s->get_local_path().find("/") >= 0) continue;
            if (s->get_token() == p_symbol) {
                symbol = s;
                break;
            }
        }
    }
    if (!symbol.is_valid()) return result;
    Array clips = symbol->get_clips().keys();
    for (int i=0; i<clips.size(); i++){
        String clip_name = clips[i];
        result.push_back(clip_name);
    }
    return result;
}

void FlashPlayer::queue_process(float p_delta) {
    queued_delta = MAX(p_delta, queued_delta);
    if (!animation_process_queued) {
        animation_process_queued = true;
        call_deferred("_animation_process");
    }
}

void FlashPlayer::_animation_process() {
    if (processed_frame == frame && !tracks_dirty) {
        animation_process_queued = false;
        queued_delta = 0.0;
        return;
    }
    events.clear();
    masks.clear();
    clipping_cache.clear();
    clipping_items.clear();
    metaball_cache.clear();
    metaball_rect = Rect2();
    processed_frame = frame;
    indices.resize(0);
    points.resize(0);
    colors.resize(0);
    uvs.resize(0);

    if (!active_symbol.is_valid()) {
        update();
        animation_process_queued = false;
        queued_delta = 0.0;
        tracks_dirty = false;
        return;
    }

    active_symbol->animation_process(this, frame, queued_delta);
    update();
    performance_triangles_generated = indices.size() / 3;

    for (List<String>::Element *E = events.front(); E; E = E->next()) {
        // always emit user events in deferred mode
        // to prevent recursive `animation_process` invocation
#ifndef TOOLS_ENABLED
        call_deferred("emit_signal", "animation_event", E->get());
#else
        if (!Engine::get_singleton()->is_editor_hint())
            call_deferred("emit_signal", "animation_event", E->get());
#endif
    }
    animation_process_queued = false;
    queued_delta = 0.0;
    tracks_dirty = false;
}

void FlashPlayer::advance(float p_time, bool p_seek, bool advance_all_frames) {
    if (!active_symbol.is_valid()) return;
    emit_signal("advance_called", p_time, p_seek, advance_all_frames);
    bool animation_completed = false;
    float delta = p_time*frame_rate;
    if (p_seek) {
        frame = playback_start + delta;
    } else {
        frame += delta;
    }

    
    if (advance_all_frames) {
        List<String> clip_keys;
        clips_state.get_key_list(&clip_keys);
        for (List<String>::Element *E = clip_keys.front(); E; E = E->next()) {
            String clips_key = E->get();
            Vector3 *clip = clips_state.getptr(clips_key);
            if (clip == NULL) {
                continue;
            }
            if (p_seek) {
                clip->z = delta;
            } else {
                clip->z += delta;
            }
            float duration = clip->y - clip->x;
            if (duration <= 0) {
                clip->z = 0.0;
            } else if (loop) while (clip->z > duration) {
                clip->z -= duration;
            }
        }
    }

    if (p_seek) {
        delta = 0.0;
    }

    float duration = playback_end - playback_start;
    if (!loop && frame > playback_end) {
        animation_completed = true;
        frame = playback_end - 0.0001;
    } else if (loop && duration >= 0) while (frame > playback_end) {
        animation_completed = true;
        frame -= playback_end - playback_start;
    }
    queue_process(delta);
    if (animation_completed) {
#ifndef TOOLS_ENABLED
        call_deferred("emit_signal", "animation_completed");
#else
        if (!Engine::get_singleton()->is_editor_hint())
            call_deferred("emit_signal", "animation_completed");
#endif

    }
}

void FlashPlayer::advance_clip_for_track_wrapper(const String &p_track, const String &p_clip, float p_time, bool p_seek) {
    float r_elapsed, r_remaining;
    advance_clip_for_track(p_track, p_clip, p_time, p_seek, &r_elapsed, &r_remaining);
}


void FlashPlayer::advance_clip_for_track(const String &p_track, const String &p_clip, float p_time, bool p_seek, float *r_elapsed, float *r_remaining) {
    if (!resource.is_valid()) return;
    emit_signal("advance_clip_for_track_called", p_track, p_clip, p_time, p_seek);
    if (p_clip == Variant() || p_clip == "[default]") {
        if(clips_state.has(p_track)) clips_state.erase(p_track);
        if(active_clips.has(p_track)) active_clips.erase(p_track);
        if (r_elapsed != NULL) *r_elapsed = 0.0;
        if (r_remaining != NULL) *r_remaining = 0.0;
        return;
    }

    float delta = p_time*frame_rate;
    String *current_clip = active_clips.getptr(p_track);
    if (current_clip == NULL || *current_clip != p_clip) {
        active_clips[p_track] = p_clip;
        Array timelines = resource->get_symbols().values();
        for (int i=0; i<timelines.size(); i++) {
            Ref<FlashTimeline> tl = timelines[i];
            if (p_track != tl->get_clips_header()) continue;
            Vector2 clip_data = tl->get_clips()[p_clip];
            clips_state[p_track] = Vector3(clip_data.x, clip_data.y, 0.0);
            break;
        }
    }

    Vector3 *current_state = clips_state.getptr(p_track);
    if (current_state == NULL) {
        if (r_elapsed != NULL) *r_elapsed = 0.0;
        if (r_remaining != NULL) *r_remaining = 0.0;
        return;
    }
    float duration = current_state->y - current_state->x;
    if (p_seek) {
        if (current_state->z != delta) {
            tracks_dirty = true;
            current_state->z = delta;
        }
    } else {
        float next_state = MIN(duration, current_state->z + delta);
        if (duration <= 0.0) {
            next_state = 0.0;
        } else if (loop) while (next_state > duration) {
            next_state -= duration;
        }
        if (next_state != current_state->z) {
            tracks_dirty = true;
            current_state->z = next_state;
        }
    }
    queue_process();
    if (r_elapsed != NULL) *r_elapsed = MIN(duration, current_state->z) / frame_rate;
    if (r_remaining != NULL) *r_remaining = (duration - current_state->z) / frame_rate;
}

void FlashPlayer::add_polygon(Vector<Vector2> p_points, Vector<Color> p_colors, Vector<Vector2> p_uvs, int p_texture_idx) {
    // Array pt
    Vector2 s = p_points[2] - p_points[0];
    float area = s.x * s.y;
    
    Vector<int> local_indices = Geometry::triangulate_polygon(p_points);
    for (int i=0; i<local_indices.size(); i++){
        indices.push_back(local_indices[i] + points.size());
    }
    int clipping_id = clipping_cache.size();
    int clipping_size_with_tex_idx = (clipping_items.size() << 8) | (p_texture_idx & 0xff);
    for (int i=0; i<p_points.size(); i++) {
        points.push_back(p_points[i]);
        colors.push_back(p_colors[i]);
        uvs.push_back(p_uvs[i] * 0.5 + Vector2(clipping_id, clipping_size_with_tex_idx));
    }
}

void FlashPlayer::add_metaball(const Vector2 &p_pos, const float &p_radius) {
    if (is_masking()) {
        return;
    }
    if (metaball_cache.size() == 0) {
        metaball_rect = Rect2(p_pos - Vector2(p_radius, p_radius), Vector2(p_radius * 2.0, p_radius * 2.0));    
    } else {
        metaball_rect = metaball_rect.expand(p_pos - Vector2(p_radius, p_radius));
        metaball_rect = metaball_rect.expand(p_pos + Vector2(p_radius, p_radius));
    }
    metaball_cache.push_back(Vector3(p_pos.x, p_pos.y, p_radius));
}

void FlashPlayer::queue_animation_event(const String &p_event, bool p_reversed) {
    if (events.find(p_event) == NULL) {
        if (p_reversed) {
            events.push_front(p_event);
        } else {
            events.push_back(p_event);
        }
    }
}

void FlashPlayer::update_clipping_data() {
    clipping_data->lock();
    Vector2i pos = Vector2i(0, 0);
    Transform2D scale;
    //scale.scale(Vector2(2.0, 2.0));

    Transform2D glob = get_viewport_transform() * get_global_transform_with_canvas();
    for (List<FlashMaskItem>::Element *E = clipping_cache.front(); E; E = E->next()) {
        FlashMaskItem item = E->get();
        Transform2D tr = (glob * item.transform * scale).affine_inverse();
        Color xy = Color(tr[0].x, tr[0].y, tr[1].x, tr[1].y);
        Color origin = Color(tr[2].x, tr[2].y, item.texture_idx, 0);
        Color region = Color(
            item.texture_region.position.x,
            item.texture_region.position.y,
            item.texture_region.size.width,
            item.texture_region.size.height
        );
        clipping_data->set_pixel(pos.x, pos.y, xy);
        clipping_data->set_pixel(pos.x+1, pos.y, origin);
        clipping_data->set_pixel(pos.x+2, pos.y, region);
        pos.x += 4;
        if (pos.x >= 32) {
            pos.x = 0;
            pos.y += 1;
            if (pos.y >= 32) break;
        }
    }
    clipping_data->unlock();
    clipping_texture->set_data(clipping_data);
}

void FlashPlayer::mask_begin(int mask_id) {
    if (!current_mask) current_mask = mask_id;
    masks.set(current_mask, List<FlashMaskItem>());
    mask_stack.push_back(mask_id);
}
void FlashPlayer::mask_end(int mask_id) {
    if (current_mask == mask_id) {
        mask_stack.pop_front();
        if (mask_stack.size() > 0) {
            current_mask = mask_stack.back()->get();
        } else {
            current_mask = 0;
        }
    }
}
bool FlashPlayer::is_masking() {
    return current_mask > 0;
}
void FlashPlayer::mask_add(Transform2D p_transform, Rect2i p_texture_region, int p_texture_idx) {
    FlashMaskItem item;
    item.texture_idx = p_texture_idx;
    item.texture_region = p_texture_region;
    item.transform = p_transform;
    if (!masks.has(current_mask)) {
        masks.set(current_mask, List<FlashMaskItem>());
    }
    masks[current_mask].push_back(item);
}
void FlashPlayer::clip_begin(int mask_id) {
    if (!masks.has(mask_id)) {
        print_line("No flash mask found, id=" + itos(mask_id));
        return;
    }
    for (List<FlashMaskItem>::Element *E = clipping_items.front(); E; E = E->next()) {
        clipping_cache.push_back(E->get());
    }
    for (List<FlashMaskItem>::Element *E = masks[mask_id].front(); E; E = E->next()) {
        clipping_items.push_back(E->get());
    }
}
void FlashPlayer::clip_end(int mask_id) {
    if (!masks.has(mask_id)) return;
    for (List<FlashMaskItem>::Element *E = clipping_items.front(); E; E = E->next()) {
        clipping_cache.push_back(E->get());
    }
    for (List<FlashMaskItem>::Element *E = masks[mask_id].front(); E; E = E->next()) {
        clipping_items.pop_back();
    }
}

FlashPlayer::~FlashPlayer() {
    VisualServer *vs = VisualServer::get_singleton();
    vs->free(flash_material);
    vs->free(mesh);
}

FlashPlayer::FlashPlayer() {
    frame = 0;
    frame_rate = 30;
    queued_delta = 0.0;
    playing = false;
    playback_start = 0;
    playback_end = 0;
    active_symbol_name = "[document]";
    active_clip = "";
    loop = false;
    tracks_dirty = true;
    animation_process_queued = false;

    processed_frame = -1;
    current_mask = 0;
    cliping_depth = 0;

    performance_triangles_generated = 0;
    performance_triangles_drawn = 0;

    clipping_texture.instance();
    clipping_data.instance();
    clipping_data->create(32, 32, false, Image::FORMAT_RGBAH);
    clipping_texture->create_from_image(clipping_data);

    VisualServer *vs = VisualServer::get_singleton();
    flash_material = vs->material_create();
    vs->canvas_item_set_material(get_canvas_item(), flash_material);
    mesh = vs->mesh_create();
    _generate_normal_shader();
    _generate_metaball_shader();
    VisualServer::get_singleton()->material_set_shader(flash_material, normal_shader);
    VisualServer::get_singleton()->material_set_param(flash_material, "CLIPPING_TEXTURE", clipping_texture);
    render_mode = RENDER_NORMAL;
    metaball_threshold = 1.0;
    metaball_debug = false;
    metaball_weight_balancing = WEIGHT_BALANCE_EXPONENTIAL;
    metaball_color = Color(0.415686, 0.352941, 0.788235);
    // vs->material_set_shader(flash_material, normal_shader);
}
#endif