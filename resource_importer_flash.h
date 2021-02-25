#ifdef MODULE_FLASH_ENABLED
#ifndef RESOURCE_IMPORTER_FLASH_H
#define RESOURCE_IMPORTER_FLASH_H

#include <editor/import/resource_importer_texture.h>

class ResourceImporterFlash: public ResourceImporter {
    GDCLASS(ResourceImporterFlash, ResourceImporter);

    static const int importer_version;
	static const char *compression_formats[];

protected:
	Error _save_tex(
		const String &p_path,
		const Vector<Ref<Image>> &p_spritesheets,
		int p_compress_mode,
		Image::CompressMode p_vram_compression,
		bool p_mipmaps,
		int p_texture_flags
	);

public:
	enum CompressMode {
		COMPRESS_LOSSLESS,
		COMPRESS_VIDEO_RAM,
		COMPRESS_UNCOMPRESSED
	};

	virtual String get_importer_name() const;
	virtual String get_visible_name() const;
	virtual int get_preset_count() const { return 1; }
	virtual String get_preset_name(int p_idx) const { return "Default"; }
	virtual bool get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const { return true; }
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual String get_save_extension() const;
	virtual void get_import_options(List<ImportOption> *r_options, int p_preset) const;
	virtual bool are_import_settings_valid(const String &p_path) const;
	virtual String get_import_settings_string() const;

	virtual String get_resource_type() const;
#ifdef GODOT_FEATURE_IMPORTER_VERSION
	virtual
#endif
	int get_importer_version() const;

    virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = NULL, Variant *r_metadata = NULL);

};



#endif
#endif