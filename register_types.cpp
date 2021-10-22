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

#include <core/class_db.h>
#include <core/project_settings.h>
#include "register_types.h"
#include "flash_player.h"
#include "flash_resources.h"
#include "animation_node_flash.h"

#ifdef TOOLS_ENABLED
#include "core/engine.h"
#include "editor/editor_export.h"
#include "editor/editor_node.h"
#include "resource_importer_flash.h"

class EditorExportFlash : public EditorExportPlugin {

	GDCLASS(EditorExportFlash, EditorExportPlugin);
	bool export_processed;

public:
	virtual void _export_begin(const Set<String> &p_features, bool p_debug, const String &p_path, int p_flags) {
		export_processed = false;
	}


	virtual void _export_file(const String &p_path, const String &p_type, const Set<String> &p_features) {
		if (p_type != "FlashDocument") return;
		Ref<ConfigFile> config;
		config.instance();
		Error err = config->load(p_path + ".import");
		if (err != OK) return;

		List<String> remaps;
		config->get_section_keys("remap", &remaps);

		Set<String> remap_features;

		for (List<String>::Element *F = remaps.front(); F; F = F->next()) {

			String remap = F->get();
			String feature = remap.get_slice(".", 1);
			if (p_features.has(feature)) {
				remap_features.insert(feature);
			}
		}
		String imported_doc_path;
		for (List<String>::Element *F = remaps.front(); F; F = F->next()) {
			String remap = F->get();
			if (remap == "path") {
				String imported_doc_path = config->get_value("remap", remap);
				String texture_path = imported_doc_path.substr(0, imported_doc_path.length()-3) + "ftex";
				add_file(texture_path, FileAccess::get_file_as_array(texture_path), false);
			} else if (remap.begins_with("path.")) {
				String feature = remap.get_slice(".", 1);
				if (remap_features.has(feature)) {
					String imported_doc_path = config->get_value("remap", remap);
					String texture_path = imported_doc_path.substr(0, imported_doc_path.length()-3) + "ftex";
					add_file(texture_path, FileAccess::get_file_as_array(texture_path), false);
				}
			}
		}
	}
};

static void _editor_init() {
	Ref<ResourceImporterFlash> flash_import;
	flash_import.instance();
	ResourceFormatImporter::get_singleton()->add_importer(flash_import);

	Ref<EditorExportFlash> flash_export;
	flash_export.instance();
	EditorExport::get_singleton()->add_export_plugin(flash_export);
}
#endif


Ref<ResourceFormatLoaderFlashTexture> resource_loader_flash_texture;

void register_flash_types() {
	// core flash classes
	ClassDB::register_class<FlashPlayer>();
	ClassDB::register_class<FlashMachine>();
	ClassDB::register_class<AnimationNodeFlashClip>();

	// resources
	ClassDB::register_virtual_class<FlashElement>();
	ClassDB::register_class<FlashTextureRect>();
	ClassDB::register_class<FlashDocument>();
	ClassDB::register_class<FlashBitmapItem>();
	ClassDB::register_class<FlashTimeline>();
	ClassDB::register_class<FlashLayer>();
	ClassDB::register_class<FlashFrame>();
	ClassDB::register_class<FlashDrawing>();
	ClassDB::register_class<FlashInstance>();
	ClassDB::register_class<FlashGroup>();
	ClassDB::register_class<FlashShape>();
	ClassDB::register_class<FlashBitmapInstance>();
	ClassDB::register_class<FlashTween>();


	// loader
	resource_loader_flash_texture.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_flash_texture);

#ifdef TOOLS_ENABLED
	EditorNode::add_init_callback(_editor_init);
#endif
}

void unregister_flash_types() {
	ResourceLoader::remove_resource_format_loader(resource_loader_flash_texture);
	resource_loader_flash_texture.unref();
}

#else

void register_flash_types() {}
void unregister_flash_types() {}

#endif // MODULE_SPINE_ENABLED
