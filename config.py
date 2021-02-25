

def can_build(platform):
  return True
  
  
def configure(env):
  godot_feature_importer_version = False
  with open("editor/import/editor_import_plugin.h", 'r') as f:
    godot_feature_importer_version = "get_importer_version" in f.read()
  if godot_feature_importer_version:
    env.Append(CPPFLAGS=['-DGODOT_FEATURE_IMPORTER_VERSION'])
    print("Importer Version supported")
  
  
  
