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

#include <core/io/dir_access.h>
#include <core/io/file_access.h>
#include <core/io/compression.h>
#include <core/io/image_loader.h>
#include <core/io/resource_importer.h>
#include <core/io/zip_io.h>
#include <core/math/geometry_2d.h>
#include <core/io/json.h>
#include <core/io/marshalls.h>
#include <core/config/project_settings.h>
#include <editor/import/resource_importer_texture.h>
#include <editor/import/resource_importer_layered_texture.h>
#include <editor/import/resource_importer_texture_settings.h>
#include <scene/resources/compressed_texture.h>

#include "resource_importer_flash.h"
#include "flash_resources.h"

const int ResourceImporterFlash::importer_version = 12;

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
	return "tres";
}

String ResourceImporterFlash::get_resource_type() const {
	return "FlashDocument";
}

int ResourceImporterFlash::get_importer_version() const {
    return ResourceImporterFlash::importer_version;
}

void ResourceImporterFlash::get_import_options(const String &p_path, List<ResourceImporter::ImportOption> *r_options, int p_preset) const {
// void ResourceImporterFlash::get_import_options(List<ImportOption> *r_options, int p_preset) const {

    r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "process/downscale", PROPERTY_HINT_ENUM, "Disabled,x2,x4"), 0));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "process/fix_alpha_border"), true));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "compress/mode", PROPERTY_HINT_ENUM, "Lossless (PNG),Video RAM (S3TC/ETC/BPTC),Uncompressed", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), 1));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "flags/repeat", PROPERTY_HINT_ENUM, "Disabled,Enabled,Mirrored"), 0));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "flags/filter"), true));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "flags/mipmaps"), true));
	r_options->push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "flags/srgb", PROPERTY_HINT_ENUM, "Disable,Enable"), 0));
}

bool ResourceImporterFlash::get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const {
    return true;
}

bool ResourceImporterFlash::are_import_settings_valid(const String &p_path) const {

	//will become invalid if formats are missing to import
// 	Dictionary metadata = ResourceFormatImporter::get_singleton()->get_resource_metadata(p_path);

// 	if (!metadata.has("vram_texture")) {
// 		return false;
// 	}

// #ifndef GODOT_FEATURE_IMPORTER_VERSION
//     int imported_with_version = metadata.get("importer_version", 0);
//     if (imported_with_version != get_importer_version()) {
//         return false;
//     }
// #endif

// 	bool vram = metadata["vram_texture"];
// 	if (!vram) {
// 		return true; //do not care about non vram
// 	}

// 	Vector<String> formats_imported;
// 	if (metadata.has("imported_formats")) {
// 		formats_imported = metadata["imported_formats"];
// 	}

// 	int index = 0;
// 	bool valid = true;
// 	while (compression_formats[index]) {
// 		String setting_path = "rendering/vram_compression/import_" + String(compression_formats[index]);
// 		bool test = ProjectSettings::get_singleton()->get(setting_path);
// 		if (test) {
// 			if (formats_imported.find(compression_formats[index]) == -1) {
// 				valid = false;
// 				break;
// 			}
// 		}
// 		index++;
// 	}

	return true;
}

Error ResourceImporterFlash::import(const String &p_source_file, const String &p_save_path, const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
    int compress_mode = p_options["compress/mode"];
	int repeat = p_options["flags/repeat"];
	bool filter = p_options["flags/filter"];
	bool mipmaps = p_options["flags/mipmaps"];
	int srgb = p_options["flags/srgb"];
    int downscale = p_options["process/downscale"];
    bool fix_alpha_border = p_options["process/fix_alpha_border"];
    bool high_quality = true;//p_options["compress/high_quality"];
    // int32_t tex_flags = ImageFormatLoader::FLAG_NONE;
	// if (repeat > 0)
	// 	tex_flags |= ImageFormatLoader::FLAG_REPEAT;
	// if (repeat == 2)
	// 	tex_flags |= ImageFormatLoader::FLAG_MIRRORED_REPEAT;
	// if (filter)
	// 	tex_flags |= ImageFormatLoader::FLAG_FILTER;
	// if (mipmaps || compress_mode == COMPRESS_VIDEO_RAM)
	// 	tex_flags |= ImageFormatLoader::FLAG_MIPMAPS;
	// if (srgb == 1)
	// 	tex_flags |= ImageFormatLoader::FLAG_FORCE_LINEAR;

    // read zip and extract it to tmp dir
    //const Vector2 PADDING(1, 1);
    Ref<FileAccess> zip_source_file;
    // zlib_filefunc_def io = zipio_open(&zip_source_file);
    zlib_filefunc_def io = zipio_create_io(&zip_source_file);
    zipFile zip_source = unzOpen2(p_source_file.utf8().get_data(), &io);
    if (zip_source == NULL) return FAILED;

    Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
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
        PackedByteArray data;
        data.resize(info.uncompressed_size);
        if (unzOpenCurrentFile(zip_source) != UNZ_OK) {
		    ERR_FAIL_V_MSG(FAILED, "Could not open file within zip archive.");
	    }
        unzReadCurrentFile(zip_source, data.ptrw(), info.uncompressed_size);
        String file_path = tmp_dir + file_name;
        da->make_dir_recursive(file_path.get_base_dir());
        Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::WRITE);
        file->store_buffer(data.ptr(), data.size());
        file->close();
        unzCloseCurrentFile(zip_source);
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
        spritesheet_files.remove_at(spritesheet_files.size()-1);
    }

    Vector<Ref<Image>> spritesheet_images;
    Dictionary spritesheets_layout;
    for (int i=0; i<spritesheet_files.size(); i++) {
        String spriteheet_base_path = doc->get_document_path() + "/" + spritesheet_files[i];
        String json_err_msg;
        int json_err_line;
        String json_text = FileAccess::get_file_as_string(spriteheet_base_path + ".json");
        Variant json_variant = JSON::parse_string(json_text);
        Dictionary json = json_variant;
        Dictionary frames = json["frames"];
        for (int j=0; j<frames.size(); j++){
            Dictionary frame_info = frames.get_value_at_index(j);
            frame_info["texture_idx"] = i;
            spritesheets_layout[(String)frames.get_key_at_index(j)] = frame_info;
        }
        String spriteheet_path = spriteheet_base_path + ".png";
        Ref<Image> img;
        img.instantiate();
        img->load(spriteheet_path);
        if (img->get_format() != Image::FORMAT_RGBA8) {
            img->convert(Image::FORMAT_RGBA8);
        }
        if (fix_alpha_border) {
            img->fix_alpha_edges();
        }
        for (int j=0; j<downscale; j++) {
            img->shrink_x2();
            if (fix_alpha_border) {
                img->fix_alpha_edges();
            }
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
        frame.instantiate();

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

    const bool can_s3tc_bptc = ResourceImporterTextureSettings::should_import_s3tc_bptc();
	const bool can_etc2_astc = ResourceImporterTextureSettings::should_import_etc2_astc();
    String extension = get_save_extension();
    Array formats_imported;
    if (compress_mode == COMPRESS_VIDEO_RAM) {
		if (can_s3tc_bptc) {
			Image::CompressMode image_compress_mode;
			String image_compress_format;
			if (high_quality) {
				image_compress_mode = Image::COMPRESS_BPTC;
				image_compress_format = "bptc";
			} else {
				image_compress_mode = Image::COMPRESS_S3TC;
				image_compress_format = "s3tc";
			}
			_save_tex(spritesheet_images, p_save_path + "." + image_compress_format + ".ctexarray", compress_mode, 0.95, image_compress_mode, Image::COMPRESS_SOURCE_GENERIC, Image::USED_CHANNELS_RGBA, mipmaps, true);
            Ref<CompressedTexture2DArray> atlas = ResourceLoader::load(p_save_path + "." + image_compress_format + ".ctexarray");
            doc->set_atlas(atlas);
            ResourceSaver::save(doc, p_save_path + "." + image_compress_format + "." + extension);
			r_platform_variants->push_back(image_compress_format);
		}
        if (can_etc2_astc) {
			Image::CompressMode image_compress_mode;
			String image_compress_format;
			if (high_quality) {
				image_compress_mode = Image::COMPRESS_ASTC;
				image_compress_format = "astc";
			} else {
				image_compress_mode = Image::COMPRESS_ETC2;
				image_compress_format = "etc2";
			}
			_save_tex(spritesheet_images, p_save_path + "." + image_compress_format + ".ctexarray", compress_mode, 0.95, image_compress_mode, Image::COMPRESS_SOURCE_GENERIC, Image::USED_CHANNELS_RGBA, mipmaps, true);
            Ref<CompressedTexture2DArray> atlas = ResourceLoader::load(p_save_path + "." + image_compress_format + ".ctexarray");
            doc->set_atlas(atlas);
            ResourceSaver::save(doc, p_save_path + "." + image_compress_format + "." + extension);
			r_platform_variants->push_back(image_compress_format);
		}
	} else {
        _save_tex(spritesheet_images, p_save_path + ".ctexarray", compress_mode, 0.95, Image::COMPRESS_S3TC /* IGNORED */, Image::COMPRESS_SOURCE_GENERIC,  Image::USED_CHANNELS_RGBA, mipmaps, false);
        Ref<CompressedTexture2DArray> atlas = ResourceLoader::load(p_save_path + ".ctexarray");
            doc->set_atlas(atlas);
        ResourceSaver::save(doc, p_save_path + "." + extension);
	}

	if (r_metadata) {
		Dictionary metadata;
		metadata["vram_texture"] = compress_mode == COMPRESS_VIDEO_RAM;
		if (formats_imported.size()) {
			metadata["imported_formats"] = formats_imported;
		}
		*r_metadata = metadata;
	}

	return OK;








//     // for (List<String>::Element *E = formats.front(); E; E = E->next()) {
//     //     String fmt = E->get();
//     //     String stex_variant_path = atlas_imported_path + fmt.substr(0, fmt.length()-4) + "stex";
//     //     String atlas_variant_path = atlas_imported_path + fmt;
//     //     da->rename(stex_variant_path, atlas_variant_path);
//     //     Ref<Texture> atlas_texture = ResourceLoader::load(atlas_variant_path);
//     //     for (int i=0; i<items.size(); i++) {
//     //         Ref<AtlasTexture> texture; texture.instance();
//     //         texture->set_atlas(atlas_texture);
//     //         texture->set_region(Rect2(positions[i] + PADDING, sizes[i] - 2 * PADDING));
//     //         Ref<FlashBitmapItem> item = items[i];
//     //         item->set_texture(texture);
//     //     }
//     //     String save_path = p_save_path + fmt.substr(0, fmt.length()-4) + get_save_extension();
//     //     ResourceSaver::save(save_path, doc);
//     // }

//     // da->remove(tmp_dir);

    return OK;
}

void ResourceImporterFlash::_save_tex(Vector<Ref<Image>> p_images, const String &p_to_path, int p_compress_mode, float p_lossy, Image::CompressMode p_vram_compression, Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps, bool p_force_po2) {
    Vector<Ref<Image>> mipmap_images; //for 3D
    for (int i = 0; i < p_images.size(); i++) {
        if (p_force_po2) {
            p_images.write[i]->resize_to_po2();
        }

        if (p_mipmaps) {
            p_images.write[i]->generate_mipmaps(p_csource == Image::COMPRESS_SOURCE_NORMAL);
        } else {
            p_images.write[i]->clear_mipmaps();
        }
    }
    Ref<FileAccess> f = FileAccess::open(p_to_path, FileAccess::WRITE);
	f->store_8('G');
	f->store_8('S');
	f->store_8('T');
	f->store_8('L');

	f->store_32(CompressedTextureLayered::FORMAT_VERSION);
	f->store_32(p_images.size()); // For 2d layers or 3d depth.
	f->store_32(ResourceImporterLayeredTexture::MODE_2D_ARRAY);
	f->store_32(0);

	f->store_32(0);
	f->store_32(mipmap_images.size()); // Adjust the amount of mipmaps.
	f->store_32(0);
	f->store_32(0);

	for (int i = 0; i < p_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, p_images[i], ResourceImporterTexture::CompressMode(p_compress_mode), used_channels, p_vram_compression, p_lossy);
	}

	for (int i = 0; i < mipmap_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, mipmap_images[i], ResourceImporterTexture::CompressMode(p_compress_mode), used_channels, p_vram_compression, p_lossy);
	}
}
// Error ResourceImporterFlash::_save_tex(
//     const String &p_path,
//     const Vector<Ref<Image>> &p_spritesheets,
//     int p_compress_mode,
//     Image::CompressMode p_vram_compression,
//     bool p_mipmaps,
//     int p_texture_flags
// ) {
//     Error error;

//     Dictionary info;
//     info["flags"] = p_texture_flags;
//     if (p_spritesheets.size() == 0){
//         info["width"] = 0;
//         info["height"] = 0;
//         info["format"] = Image::FORMAT_RGBA8;
//     } else {
//         info["format"] = p_spritesheets[0]->get_format();
//         info["width"] = p_spritesheets[0]->get_width();
//         info["height"] = p_spritesheets[0]->get_height();
//     }

//     Array images;
//     for (int i = 0; i < p_spritesheets.size(); i++) {
//         Ref<Image> image = p_spritesheets[i]->duplicate();
// 		switch (p_compress_mode) {
//             case COMPRESS_UNCOMPRESSED:
// 			case COMPRESS_LOSSLESS: {
// 				image->clear_mipmaps();
//                 images.push_back(image);

// 			} break;
// 			case COMPRESS_VIDEO_RAM: {
// 				image->generate_mipmaps(false);
// 				Image::CompressSource csource = Image::COMPRESS_SOURCE_NORMAL;
// 				image->compress(p_vram_compression, csource);
//                 images.push_back(image);
// 			} break;
// 		}
//         info["format"] = image->get_format();
// 	}
//     info["images"] = images;
//     PoolByteArray buff;
//     Variant info_var = info;
//     int len;
//     encode_variant(info_var, NULL, len, true);
//     buff.resize(len);
//     {
//         PoolByteArray::Write w = buff.write();
//         encode_variant(info_var, w.ptr(), len, true);
//     }

//     PoolVector<uint8_t> compressed;
//     compressed.resize(Compression::get_max_compressed_buffer_size(len, Compression::MODE_FASTLZ));
//     int compressed_size = Compression::compress(compressed.write().ptr(), buff.read().ptr(), len, Compression::MODE_FASTLZ);

// 	FileAccess *f = FileAccess::open(p_path, FileAccess::WRITE, &error);
// 	ERR_FAIL_COND_V(error, error);
//     {
//         f->store_32(len);
//         PoolByteArray::Read r = compressed.read();
//         f->store_buffer(r.ptr(), compressed_size);
//     }
//     memdelete(f);
//     return OK;
// }

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