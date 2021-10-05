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

	void _edit_files_with_filter(DirAccess *da, const Vector<String> &p_filters, Set<String> &r_list, bool exclude) {

	da->list_dir_begin();
	String cur_dir = da->get_current_dir().replace("\\", "/");
	if (!cur_dir.ends_with("/"))
		cur_dir += "/";
	String cur_dir_no_prefix = cur_dir.replace("res://", "");

	Vector<String> dirs;
	String f;
	while ((f = da->get_next()) != "") {
		if (da->current_is_dir())
			dirs.push_back(f);
		else {
			String fullpath = cur_dir + f;
			// Test also against path without res:// so that filters like `file.txt` can work.
			String fullpath_no_prefix = cur_dir_no_prefix + f;
			for (int i = 0; i < p_filters.size(); ++i) {
				if (fullpath.matchn(p_filters[i]) || fullpath_no_prefix.matchn(p_filters[i])) {
					if (!exclude) {
						r_list.insert(fullpath);
					} else {
						r_list.erase(fullpath);
					}
				}
			}
		}
	}

	da->list_dir_end();

	for (int i = 0; i < dirs.size(); ++i) {
		String dir = dirs[i];
		if (dir.begins_with("."))
			continue;
		da->change_dir(dir);
		_edit_files_with_filter(da, p_filters, r_list, exclude);
		da->change_dir("..");
	}
}

	void _edit_filter_list(Set<String> &r_list, const String &p_filter, bool exclude) {

		if (p_filter == "")
			return;
		Vector<String> split = p_filter.split(",");
		Vector<String> filters;
		for (int i = 0; i < split.size(); i++) {
			String f = split[i].strip_edges();
			if (f.empty())
				continue;
			filters.push_back(f);
		}

		DirAccess *da = DirAccess::open("res://");
		ERR_FAIL_NULL(da);
		_edit_files_with_filter(da, filters, r_list, exclude);
		memdelete(da);
	}

	void _export_find_resources(EditorFileSystemDirectory *p_dir, Set<String> &p_paths) {

		for (int i = 0; i < p_dir->get_subdir_count(); i++) {
			_export_find_resources(p_dir->get_subdir(i), p_paths);
		}

		for (int i = 0; i < p_dir->get_file_count(); i++) {
			p_paths.insert(p_dir->get_file_path(i));
		}
	}

	void _export_find_dependencies(const String &p_path, Set<String> &p_paths) {

		if (p_paths.has(p_path))
			return;

		p_paths.insert(p_path);

		EditorFileSystemDirectory *dir;
		int file_idx;
		dir = EditorFileSystem::get_singleton()->find_file(p_path, &file_idx);
		if (!dir)
			return;

		Vector<String> deps = dir->get_file_deps(file_idx);

		for (int i = 0; i < deps.size(); i++) {

			_export_find_dependencies(deps[i], p_paths);
		}
	}

	virtual void _export_file(const String &p_path, const String &p_type, const Set<String> &p_features) {
		if (export_processed) return;
		export_processed = true;

		Set<String> paths;
		Ref<EditorExportPreset> preset = get_export_preset();

		if (preset->get_export_filter() == EditorExportPreset::EXPORT_ALL_RESOURCES) {
			//find stuff
			_export_find_resources(EditorFileSystem::get_singleton()->get_filesystem(), paths);
		} else {
			bool scenes_only = preset->get_export_filter() == EditorExportPreset::EXPORT_SELECTED_SCENES;

			Vector<String> files = preset->get_files_to_export();
			for (int i = 0; i < files.size(); i++) {
				if (scenes_only && ResourceLoader::get_resource_type(files[i]) != "PackedScene")
					continue;

				_export_find_dependencies(files[i], paths);
			}
		}

		//add native icons to non-resource include list
		_edit_filter_list(paths, String("*.icns"), false);
		_edit_filter_list(paths, String("*.ico"), false);

		_edit_filter_list(paths, preset->get_include_filter(), false);
		_edit_filter_list(paths, preset->get_exclude_filter(), true);


		StringName document_type = "FlashDocument";
		List<EditorFileSystemDirectory*> dirs_to_scan;
		EditorFileSystemDirectory* root = EditorFileSystem::get_singleton()->get_filesystem();

		dirs_to_scan.push_back(root);
		while (dirs_to_scan.size() > 0) {
			EditorFileSystemDirectory* dir = dirs_to_scan.front()->get();
			dirs_to_scan.pop_front();
			for (int i=0; i<dir->get_subdir_count(); i++) {
				dirs_to_scan.push_back(dir->get_subdir(i));
			}

			for (int i=0; i<dir->get_file_count(); i++) {
				if (dir->get_file_type(i) != document_type) continue;
				String path = dir->get_file_path(i);
				if (!paths.has(path)) continue;
				if (!FileAccess::exists(path + ".import")) continue;


				Ref<ConfigFile> config;
				config.instance();
				Error err = config->load(path + ".import");
				if (err != OK) continue;

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
						imported_doc_path = config->get_value("remap", remap);
					} else if (remap.begins_with("path.")) {
						String feature = remap.get_slice(".", 1);
						if (remap_features.has(feature)) {
							imported_doc_path = config->get_value("remap", remap);
						}
					}
				}
				if (imported_doc_path == "") continue;

				String texture_path = imported_doc_path.substr(0, imported_doc_path.length()-3) + "ftex";
				add_file(texture_path, FileAccess::get_file_as_array(texture_path), false);
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
