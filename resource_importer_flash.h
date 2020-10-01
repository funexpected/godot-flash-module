#ifdef MODULE_FLASH_ENABLED
#ifndef RESOURCE_IMPORTER_FLASH_H
#define RESOURCE_IMPORTER_FLASH_H

#include <editor/import/resource_importer_texture.h>

class ResourceImporterFlash: public ResourceImporterTexture {
    GDCLASS(ResourceImporterFlash, ResourceImporterTexture);

    static const String importer_version;

public:
	virtual String get_importer_name() const;
	virtual String get_visible_name() const;
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual String get_save_extension() const;
	virtual String get_resource_type() const;

    virtual bool are_import_settings_valid(const String &p_path) const;
    virtual Error import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files = NULL, Variant *r_metadata = NULL);

};



#endif
#endif