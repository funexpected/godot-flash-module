#ifdef MODULE_FLASH_ENABLED

#include <core/os/dir_access.h>
#include <core/os/file_access.h>
#include <core/io/zip_io.h>
#include <core/math/geometry.h>

#include "resource_importer_flash.h"
#include "flash_resources.h"

const int ResourceImporterFlash::importer_version = 1;

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

Error ResourceImporterFlash::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
    
    // read zip and extract it to tmp dir
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
        String file_name = char_filename;
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
    Array items_array = doc->get_bitmaps().values();
    Vector<Ref<FlashBitmapItem>> items;
    Vector<Size2i> sizes;
    Vector<Point2i> positions;
    Vector<Ref<Image>> images;
    Point2i atlas_size;
    for (int i=0; i<items_array.size(); i++){
        Ref<FlashBitmapItem> item = items_array[i];
        items.push_back(item);
        Ref<Image> img; img.instance();
        String img_path = doc->get_document_path() + "/" + item->get_bitmap_path();
        img->load(img_path);
        images.push_back(img);
        sizes.push_back(img->get_size() + Vector2(4,4));
    }
    Geometry::make_atlas(sizes, positions, atlas_size);
    Ref<Image> atlas; atlas.instance();
    atlas->create(atlas_size.x, atlas_size.y, false, Image::FORMAT_RGBA8);
    if (atlas_size.x > 4096 || atlas_size.y > 4096) {
        ERR_FAIL_V_MSG(FAILED, String("Generated atlas size ") + atlas_size + " bigger then maximum 4096x4096");
    }
    atlas_size = Vector2(next_power_of_2(atlas_size.x), next_power_of_2(atlas_size.y));
    
    for (int i=0; i<images.size(); i++) {
        atlas->blit_rect(images[i], Rect2(Vector2(), sizes[i] - Vector2(4,4)), positions[i] + Vector2(2,2));
    }
    String atlas_path = p_save_path + ".atlas.png";
    String atlas_imported_path = p_save_path + ".atlas";
    Error saved = atlas->save_png(atlas_path);
    atlas.unref();

    Error imported = ResourceImporterTexture::import(atlas_path, atlas_imported_path, p_options, r_platform_variants, r_gen_files, r_metadata);
    if (imported != OK) {
        da->remove(tmp_dir);
        return imported;
    }


    // save document for each texture format
    List<String> formats;
    if (r_platform_variants->size() > 0) {
        for (List<String>::Element *E = r_platform_variants->front(); E; E = E->next()) {
            formats.push_back("." + E->get() + ".ftex");
        }
    } else {
        formats.push_back(".ftex");
    }


    for (List<String>::Element *E = formats.front(); E; E = E->next()) {
        String fmt = E->get();
        String stex_variant_path = atlas_imported_path + fmt.substr(0, fmt.length()-4) + "stex";
        String atlas_variant_path = atlas_imported_path + fmt;
        da->rename(stex_variant_path, atlas_variant_path);
        Ref<Texture> atlas_texture = ResourceLoader::load(atlas_variant_path);
        for (int i=0; i<items.size(); i++) {
            Ref<AtlasTexture> texture; texture.instance();
            texture->set_atlas(atlas_texture);
            texture->set_region(Rect2(positions[i], sizes[i]));
            Ref<FlashBitmapItem> item = items[i];
            item->set_texture(texture);
        }
        String save_path = p_save_path + fmt.substr(0, fmt.length()-4) + get_save_extension();
        ResourceSaver::save(save_path, doc);
    }

    da->remove(tmp_dir);

    return OK;
}



#endif