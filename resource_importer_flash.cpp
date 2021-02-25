#ifdef MODULE_FLASH_ENABLED

#include <core/os/dir_access.h>
#include <core/os/file_access.h>
#include <core/io/compression.h>
#include <core/io/zip_io.h>
#include <core/math/geometry.h>
#include <core/io/json.h>
#include <core/io/marshalls.h>

#include "resource_importer_flash.h"
#include "flash_resources.h"

const int ResourceImporterFlash::importer_version = 10;

String ResourceImporterFlash::get_importer_name() const {
    return "flash";
}

String ResourceImporterFlash::get_visible_name() const {
    return "Flash";
}

void ResourceImporterFlash::get_recognized_extensions(List<String> *p_extensions) const {
    p_extensions->push_back("zip");
    p_extensions->push_back("zfl");
}

String ResourceImporterFlash::get_save_extension() const {
	return "res";
}

String ResourceImporterFlash::get_resource_type() const {
	return "FlashDocument";
}

int ResourceImporterFlash::get_importer_version() const {
    return ResourceImporterFlash::importer_version;
}

void ResourceImporterFlash::get_import_options(List<ImportOption> *r_options, int p_preset) const {

    r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "process/downscale", PROPERTY_HINT_ENUM, "Disabled,x2,x4"), 1));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "compress/mode", PROPERTY_HINT_ENUM, "Lossless (PNG),Video RAM (S3TC/ETC/BPTC),Uncompressed", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), 1));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "compress/no_bptc_if_rgb"), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "flags/repeat", PROPERTY_HINT_ENUM, "Disabled,Enabled,Mirrored"), 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "flags/filter"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "flags/mipmaps"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "flags/srgb", PROPERTY_HINT_ENUM, "Disable,Enable"), 0));
}

bool ResourceImporterFlash::are_import_settings_valid(const String &p_path) const {

	//will become invalid if formats are missing to import
	Dictionary metadata = ResourceFormatImporter::get_singleton()->get_resource_metadata(p_path);

	if (!metadata.has("vram_texture")) {
		return false;
	}

#ifndef GODOT_FEATURE_IMPORTER_VERSION
    int imported_with_version = metadata.get("importer_version", 0);
    if (imported_with_version != get_importer_version()) {
        return false;
    }
#endif

	bool vram = metadata["vram_texture"];
	if (!vram) {
		return true; //do not care about non vram
	}

	Vector<String> formats_imported;
	if (metadata.has("imported_formats")) {
		formats_imported = metadata["imported_formats"];
	}

	int index = 0;
	bool valid = true;
	while (compression_formats[index]) {
		String setting_path = "rendering/vram_compression/import_" + String(compression_formats[index]);
		bool test = ProjectSettings::get_singleton()->get(setting_path);
		if (test) {
			if (formats_imported.find(compression_formats[index]) == -1) {
				valid = false;
				break;
			}
		}
		index++;
	}

	return valid;
}

Error ResourceImporterFlash::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
    int compress_mode = p_options["compress/mode"];
	int no_bptc_if_rgb = p_options["compress/no_bptc_if_rgb"];
	int repeat = p_options["flags/repeat"];
	bool filter = p_options["flags/filter"];
	bool mipmaps = p_options["flags/mipmaps"];
	int srgb = p_options["flags/srgb"];
    int downscale = p_options["process/downscale"];

    int tex_flags = 0;
	if (repeat > 0)
		tex_flags |= Texture::FLAG_REPEAT;
	if (repeat == 2)
		tex_flags |= Texture::FLAG_MIRRORED_REPEAT;
	if (filter)
		tex_flags |= Texture::FLAG_FILTER;
	if (mipmaps || compress_mode == COMPRESS_VIDEO_RAM)
		tex_flags |= Texture::FLAG_MIPMAPS;
	if (srgb == 1)
		tex_flags |= Texture::FLAG_CONVERT_TO_LINEAR;

    // read zip and extract it to tmp dir
    //const Vector2 PADDING(1, 1);
    FileAccess *zip_source_file;
    zlib_filefunc_def io = zipio_create_io_from_file(&zip_source_file);
    zipFile zip_source = unzOpen2(p_source_file.utf8().get_data(), &io);
    if (zip_source == NULL) return FAILED;

    DirAccess *da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
    String tmp_dir = p_save_path + ".tmp/";
    da->make_dir_recursive(tmp_dir);

    if (unzGoToFirstFile(zip_source) != UNZ_OK) {
		return FAILED;
	}

    String document_path = "";
    do {
        char char_filename[1024];
        unz_file_info info;
		unzGetCurrentFileInfo(zip_source, &info, char_filename, sizeof(char_filename), NULL, 0, NULL, 0);
        String file_name = String::utf8(char_filename);
        if (file_name.ends_with("/")) continue;
        PoolByteArray data;
        data.resize(info.uncompressed_size);
        if (unzOpenCurrentFile(zip_source) != UNZ_OK) {
		    ERR_FAIL_V_MSG(FAILED, "Could not open file within zip archive.");
	    }
        unzReadCurrentFile(zip_source, data.write().ptr(), info.uncompressed_size);
        String file_path = tmp_dir + file_name;
        da->make_dir_recursive(file_path.get_base_dir());
        FileAccess *file = FileAccess::open(file_path, FileAccess::WRITE);
        file->store_buffer(data.read().ptr(), data.size());
        file->close();
        unzCloseCurrentFile(zip_source);
        memdelete(file);
        if (document_path == String() && file_name.get_file() == "DOMDocument.xml"){
            document_path = tmp_dir + file_name;
        }
    } while (unzGoToNextFile(zip_source) == UNZ_OK);
    if (document_path == "") {
        da->remove(tmp_dir);
        return FAILED;
    }


    // parse document
    Ref<FlashDocument> doc = FlashDocument::from_file(document_path);
    if (!doc.is_valid()) {
        da->remove(tmp_dir);
        return FAILED;
    }



    // make atlas
    // Array items_array = doc->get_bitmaps().values();
    // Vector<Ref<FlashBitmapItem>> items;
    // Vector<Size2i> sizes;
    // Vector<Point2i> positions;
    // Vector<Ref<Image>> images;
    // Point2i atlas_size;
    // for (int i=0; i<items_array.size(); i++){
    //     Ref<FlashBitmapItem> item = items_array[i];
    //     items.push_back(item);
    //     Ref<Image> img; img.instance();
    //     String img_path = doc->get_document_path() + "/" + item->get_bitmap_path();
    //     img->load(img_path);
    //     if (img->get_format() != Image::FORMAT_RGBA8) {
    //         img->convert(Image::FORMAT_RGBA8);
    //     }
    //     images.push_back(img);
    //     sizes.push_back(img->get_size() + 2 * PADDING);
    // }
    // Geometry::make_atlas(sizes, positions, atlas_size);
    // Ref<Image> atlas; atlas.instance();
    // atlas->create(atlas_size.x, atlas_size.y, false, Image::FORMAT_RGBA8);
    // if (atlas_size.x > 4096 || atlas_size.y > 4096) {
    //     ERR_FAIL_V_MSG(FAILED, String("Generated atlas size ") + atlas_size + " bigger then maximum 4096x4096");
    // }
    // atlas_size = Vector2(next_power_of_2(atlas_size.x), next_power_of_2(atlas_size.y));

    // for (int i=0; i<images.size(); i++) {
    //     atlas->blit_rect(images[i], Rect2(Vector2(), sizes[i] - 2 * PADDING), positions[i] + PADDING);
    // }
    // String atlas_path = p_save_path + ".atlas.png";
    // String atlas_imported_path = p_save_path + ".atlas";
    // Error saved = atlas->save_png(atlas_path);
    // atlas.unref();

    // Error imported = ResourceImporterTexture::import(atlas_path, atlas_imported_path, p_options, r_platform_variants, r_gen_files, r_metadata);
    // if (imported != OK) {
    //     da->remove(tmp_dir);
    //     return imported;
    // }

    String spritesheet_files_path = doc->get_document_path() + "/spritesheets.list";
    Vector<String> spritesheet_files = FileAccess::get_file_as_string(spritesheet_files_path).split("\n");
    while (spritesheet_files.size() > 0 && spritesheet_files[spritesheet_files.size()-1] == String()) {
        spritesheet_files.remove(spritesheet_files.size()-1);
    }

    Vector<Ref<Image>> spritesheet_images;
    Dictionary spritesheets_layout;
    for (int i=0; i<spritesheet_files.size(); i++) {
        String spriteheet_base_path = doc->get_document_path() + "/" + spritesheet_files[i];
        Variant json_variant;
        String json_err_msg;
        int json_err_line;
        String json_text = FileAccess::get_file_as_string(spriteheet_base_path + ".json");
        JSON::parse(json_text, json_variant, json_err_msg, json_err_line);
        Dictionary json = json_variant;
        Dictionary frames = json["frames"];
        for (int j=0; j<frames.size(); j++){
            Dictionary frame_info = frames.get_value_at_index(j);
            frame_info["texture_idx"] = i;
            spritesheets_layout[(String)frames.get_key_at_index(j)] = frame_info;
        }
        String spriteheet_path = spriteheet_base_path + ".png";
        Ref<Image> img;
        img.instance();
        img->load(spriteheet_path);
        if (img->get_format() != Image::FORMAT_RGBA8) {
            img->convert(Image::FORMAT_RGBA8);
        }
        img->fix_alpha_edges();
        for (int i=0; i<downscale; i++) {
            img->shrink_x2();
            img->fix_alpha_edges();
        }
        //img->optimize_channels();
        spritesheet_images.push_back(img);
    }

    Array items = doc->get_bitmaps().values();
    for (int i=0; i<items.size(); i++){
        Ref<FlashBitmapItem> item = items[i];
        Dictionary frame_info = spritesheets_layout.get(item->get_name().replace_first("gdexp/", ""), Dictionary());
        if (frame_info.size() == 0) continue;

        Ref<FlashTextureRect> frame;
        frame.instance();

        Dictionary frame_region = frame_info["frame"];
        Rect2 region = Rect2(
            frame_region["x"],
            frame_region["y"],
            frame_region["w"],
            frame_region["h"]
        );
        Vector2 original_size = region.size;
        for (int j=0; j<downscale; j++) {
            region = Rect2(region.position*0.5, region.size*0.5);
        }
        frame->set_region(region);
        frame->set_index(frame_info["texture_idx"]);
        frame->set_original_size(original_size);

        item->set_texture(frame);
    }

    String extension = get_save_extension();
    Array formats_imported;
    if (compress_mode == COMPRESS_VIDEO_RAM) {
		//must import in all formats, in order of priority (so platform choses the best supported one. IE, etc2 over etc).
		//Android, GLES 2.x

		bool ok_on_pc = false;
		bool encode_bptc = false;


		if (ProjectSettings::get_singleton()->get("rendering/vram_compression/import_s3tc")) {
			_save_tex(p_save_path + ".s3tc.ftex", spritesheet_images,
                compress_mode, Image::COMPRESS_S3TC, mipmaps, tex_flags);
            doc->set_atlas(ResourceLoader::load(p_save_path + ".s3tc.ftex"));
            ResourceSaver::save(p_save_path + ".s3tc." + extension, doc);
			r_platform_variants->push_back("s3tc");
			ok_on_pc = true;
			formats_imported.push_back("s3tc");
		}

		if (ProjectSettings::get_singleton()->get("rendering/vram_compression/import_etc2")) {
            _save_tex(p_save_path + ".etc2.ftex", spritesheet_images,
                compress_mode, Image::COMPRESS_ETC2, mipmaps, tex_flags);
            doc->set_atlas(ResourceLoader::load(p_save_path + ".etc2.ftex"));
            ResourceSaver::save(p_save_path + ".etc2." + extension, doc);
			r_platform_variants->push_back("etc2");
			formats_imported.push_back("etc2");
		}

		if (ProjectSettings::get_singleton()->get("rendering/vram_compression/import_etc")) {
            _save_tex(p_save_path + ".etc.ftex", spritesheet_images,
                compress_mode, Image::COMPRESS_ETC, mipmaps, tex_flags);
            doc->set_atlas(ResourceLoader::load(p_save_path + ".etc.ftex"));
            ResourceSaver::save(p_save_path + ".etc." + extension, doc);
			r_platform_variants->push_back("etc");
			formats_imported.push_back("etc");
		}

		if (ProjectSettings::get_singleton()->get("rendering/vram_compression/import_pvrtc")) {
            _save_tex(p_save_path + ".pvrtc.ftex", spritesheet_images,
                compress_mode, Image::COMPRESS_PVRTC4, mipmaps, tex_flags);
            doc->set_atlas(ResourceLoader::load(p_save_path + ".pvrtc.ftex"));
            ResourceSaver::save(p_save_path + ".pvrtc." + extension, doc);
			r_platform_variants->push_back("pvrtc");
			formats_imported.push_back("pvrtc");
		}

		if (!ok_on_pc) {
			//EditorNode::add_io_error("Warning, no suitable PC VRAM compression enabled in Project Settings. This texture will not display correctly on PC.");
		}
	} else {
		//import normally
        _save_tex(p_save_path + ".ftex", spritesheet_images,
                compress_mode, Image::COMPRESS_S3TC /*this is ignored */, mipmaps, tex_flags);
        doc->set_atlas(ResourceLoader::load(p_save_path + ".ftex"));
        ResourceSaver::save(p_save_path + "." + extension, doc);
	}

	if (r_metadata) {
		Dictionary metadata;
#ifndef GODOT_FEATURE_IMPORTER_VERSION
        metadata["importer_version"] = get_importer_version();
#endif
		metadata["vram_texture"] = compress_mode == COMPRESS_VIDEO_RAM;
		if (formats_imported.size()) {
			metadata["imported_formats"] = formats_imported;
		}
		*r_metadata = metadata;
	}

	return OK;




    // save document for each texture format
    // List<String> formats;
    // if (r_platform_variants->size() > 0) {
    //     for (List<String>::Element *E = r_platform_variants->front(); E; E = E->next()) {
    //         String format = "." + E->get() + ".ftex";
    //         if (formats.has(format)) continue;
    //         formats.push_back(format);
    //     }
    // } else {
    //     formats.push_back(".ftex");
    // }






    // for (List<String>::Element *E = formats.front(); E; E = E->next()) {
    //     String fmt = E->get();
    //     String stex_variant_path = atlas_imported_path + fmt.substr(0, fmt.length()-4) + "stex";
    //     String atlas_variant_path = atlas_imported_path + fmt;
    //     da->rename(stex_variant_path, atlas_variant_path);
    //     Ref<Texture> atlas_texture = ResourceLoader::load(atlas_variant_path);
    //     for (int i=0; i<items.size(); i++) {
    //         Ref<AtlasTexture> texture; texture.instance();
    //         texture->set_atlas(atlas_texture);
    //         texture->set_region(Rect2(positions[i] + PADDING, sizes[i] - 2 * PADDING));
    //         Ref<FlashBitmapItem> item = items[i];
    //         item->set_texture(texture);
    //     }
    //     String save_path = p_save_path + fmt.substr(0, fmt.length()-4) + get_save_extension();
    //     ResourceSaver::save(save_path, doc);
    // }

    // da->remove(tmp_dir);

    return OK;
}

Error ResourceImporterFlash::_save_tex(
    const String &p_path,
    const Vector<Ref<Image>> &p_spritesheets,
    int p_compress_mode,
    Image::CompressMode p_vram_compression,
    bool p_mipmaps,
    int p_texture_flags
) {
    Error error;

    Dictionary info;
    info["flags"] = p_texture_flags;
    if (p_spritesheets.size() == 0){
        info["width"] = 0;
        info["height"] = 0;
        info["format"] = Image::FORMAT_RGBA8;
    } else {
        info["format"] = p_spritesheets[0]->get_format();
        info["width"] = p_spritesheets[0]->get_width();
        info["height"] = p_spritesheets[0]->get_height();
    }

    Array images;
    for (int i = 0; i < p_spritesheets.size(); i++) {
        Ref<Image> image = p_spritesheets[i]->duplicate();
		switch (p_compress_mode) {
            case COMPRESS_UNCOMPRESSED:
			case COMPRESS_LOSSLESS: {
				image->clear_mipmaps();
                images.push_back(image);

			} break;
			case COMPRESS_VIDEO_RAM: {
				image->generate_mipmaps(false);
				Image::CompressSource csource = Image::COMPRESS_SOURCE_LAYERED;
				image->compress(p_vram_compression, csource, 0.7);
                images.push_back(image);
			} break;
		}
        info["format"] = image->get_format();
	}
    info["images"] = images;
    PoolByteArray buff;
    Variant info_var = info;
    int len;
    encode_variant(info_var, NULL, len, true);
    buff.resize(len);
    {
        PoolByteArray::Write w = buff.write();
        encode_variant(info_var, w.ptr(), len, true);
    }

    PoolVector<uint8_t> compressed;
    compressed.resize(Compression::get_max_compressed_buffer_size(len, Compression::MODE_FASTLZ));
    int compressed_size = Compression::compress(compressed.write().ptr(), buff.read().ptr(), len, Compression::MODE_FASTLZ);

	FileAccess *f = FileAccess::open(p_path, FileAccess::WRITE, &error);
	ERR_FAIL_COND_V(error, error);
    {
        f->store_32(len);
        PoolByteArray::Read r = compressed.read();
        f->store_buffer(r.ptr(), compressed_size);
    }
    memdelete(f);
    return OK;
}

const char *ResourceImporterFlash::compression_formats[] = {
	"s3tc",
	"etc",
	"etc2",
	"pvrtc",
	NULL
};
String ResourceImporterFlash::get_import_settings_string() const {

	String s;

	int index = 0;
	while (compression_formats[index]) {
		String setting_path = "rendering/vram_compression/import_" + String(compression_formats[index]);
		bool test = ProjectSettings::get_singleton()->get(setting_path);
		if (test) {
			s += String(compression_formats[index]);
		}
		index++;
	}

	return s;
}


#endif