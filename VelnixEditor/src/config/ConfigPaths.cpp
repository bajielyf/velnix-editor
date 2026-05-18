#include "config/ConfigPaths.h"
#include <glib.h>

static std::string build_default_config_directory(const std::string &customDir) {
  if (!customDir.empty()) {
    return customDir;
  }

  gchar *defaultDir = g_build_filename(g_get_user_config_dir(),
                                       "velnix-editor", NULL);
  std::string dir = defaultDir;
  g_free(defaultDir);
  return dir;
}

static std::string build_default_config_path(const std::string &customDir,
                                             const std::string &filename) {
  std::string dir = build_default_config_directory(customDir);
  g_mkdir_with_parents(dir.c_str(), 0700);
  std::string path = dir + "/" + filename;
  gchar *parentDir = g_path_get_dirname(path.c_str());
  if (parentDir) {
    g_mkdir_with_parents(parentDir, 0700);
    g_free(parentDir);
  }
  return path;
}

std::string get_default_config_directory(const std::string &customDir) {
  return build_default_config_directory(customDir);
}

std::string get_config_file_path(const std::string &customDir) {
  return build_default_config_path(customDir, "config.ini");
}

std::string get_shortcuts_file_path(const std::string &customDir) {
  return build_default_config_path(customDir, "shortcuts.json");
}

std::string get_macros_file_path(const std::string &customDir) {
  return build_default_config_path(customDir, "macros.txt");
}

std::string get_recent_files_file_path(const std::string &customDir) {
  return build_default_config_path(customDir, "recent_files.ini");
}

std::string get_session_file_path(const std::string &customDir) {
  return build_default_config_path(customDir, "session.json");
}
