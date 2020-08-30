#ifdef MODULE_FLASH_ENABLED

#include <core/class_db.h>
#include <core/project_settings.h>
#include "register_types.h"
#include "flash_player.h"
#include "flash_resources.h"

#ifdef TOOLS_ENABLED
#include "core/engine.h"
#include "resource_importer_flash.h"
#endif

Ref<ResourceFormatLoaderFlashTexture> resource_loader_flash_texture;

void register_flash_types() {

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		Ref<ResourceImporterFlash> importer;
		importer.instance();
		ResourceFormatImporter::get_singleton()->add_importer(importer);
	}
#endif
	ClassDB::register_class<FlashPlayer>();
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

	resource_loader_flash_texture.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_flash_texture);
}

void unregister_flash_types() {
	ResourceLoader::remove_resource_format_loader(resource_loader_flash_texture);
	resource_loader_flash_texture.unref();
}

#else

void register_flash_types() {}
void unregister_flash_types() {}

#endif // MODULE_SPINE_ENABLED
