#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "config/ConfigStore.h"
#include "config/ConfigSchema.h"

static std::string create_temp_file(const std::string &content) {
  char path_template[] = "/tmp/velnixeditor_config_test_XXXXXX";
  int fd = mkstemp(path_template);
  assert(fd != -1);
  std::string path(path_template);
  close(fd);

  std::ofstream output(path, std::ios::trunc);
  output << content;
  output.close();
  return path;
}

static void remove_file(const std::string &path) {
  if (!path.empty()) {
    std::remove(path.c_str());
  }
}

void test_load_legacy_config_keys() {
  std::string path = create_temp_file(
      "use_recent=0\n"
      "max_recent=3\n"
      "show_line_numbers=0\n"
      "config_dir=/tmp/test_settings\n"
      "last_file_dir=/tmp/last\n"
  );

  ConfigStore store(path);
  bool loaded = store.load();
  assert(loaded == true);
  assert(store.get_bool("use_recent_files") == false);
  assert(store.get_int("max_recent_files") == 3);
  assert(store.get_bool("show_line_numbers") == false);
  assert(store.get_string("config_directory") == "/tmp/test_settings");
  assert(store.get_string("last_file_dialog_dir") == "/tmp/last");

  remove_file(path);
  std::cout << "[PASS] test_load_legacy_config_keys\n";
}

void test_save_and_reload_config() {
  std::string path = create_temp_file("");

  ConfigStore store(path);
  store.set_bool("use_recent_files", true);
  store.set_int("max_recent_files", 7);
  store.set_bool("show_line_numbers", true);
  store.set_string("ui_language", "zh-Hans");
  store.set_string("line_wrap_mode", "window");
  store.set_string("font_family", "Courier");
  store.set_int("font_size", 12);
  store.set_bool("show_whitespace", true);
  store.set_bool("show_eol_markers", true);
  store.set_bool("highlight_current_line", false);
  store.set_bool("smart_keyword_highlighting", false);
  store.set_bool("show_indent_guides", false);
  store.set_int("long_line_column", 100);
  store.set_bool("scroll_past_last_line", false);
  store.set_bool("highlight_matching_braces", false);
  store.set_bool("show_right_margin_background", true);
  store.set_bool("auto_save", true);
  store.set_int("auto_save_interval", 30);
  store.set_bool("overwrite_readonly", true);
  store.set_bool("search_wrap_around", false);
  store.set_bool("search_all_documents", true);
  store.set_string("config_directory", "/tmp/newdir");
  store.set_string("last_file_dialog_dir", "/tmp/lastdir");
  store.set_string("last_update_check_date", "2026-06-22");
  bool saved = store.save();
  assert(saved == true);

  ConfigStore reload(path);
  bool loaded = reload.load();
  assert(loaded == true);
  assert(reload.get_bool("use_recent_files") == true);
  assert(reload.get_int("max_recent_files") == 7);
  assert(reload.get_bool("show_line_numbers") == true);
  assert(reload.get_string("ui_language") == "zh-Hans");
  assert(reload.get_string("line_wrap_mode") == "window");
  assert(reload.get_string("font_family") == "Courier");
  assert(reload.get_int("font_size") == 12);
  assert(reload.get_bool("show_whitespace") == true);
  assert(reload.get_bool("show_eol_markers") == true);
  assert(reload.get_bool("highlight_current_line") == false);
  assert(reload.get_bool("smart_keyword_highlighting") == false);
  assert(reload.get_bool("show_indent_guides") == false);
  assert(reload.get_int("long_line_column") == 100);
  assert(reload.get_bool("scroll_past_last_line") == false);
  assert(reload.get_bool("highlight_matching_braces") == false);
  assert(reload.get_bool("show_right_margin_background") == true);
  assert(reload.get_bool("auto_save") == true);
  assert(reload.get_int("auto_save_interval") == 30);
  assert(reload.get_bool("overwrite_readonly") == true);
  assert(reload.get_bool("search_wrap_around") == false);
  assert(reload.get_bool("search_all_documents") == true);
  assert(reload.get_string("config_directory") == "/tmp/newdir");
  assert(reload.get_string("last_file_dialog_dir") == "/tmp/lastdir");
  assert(reload.get_string("last_update_check_date") == "2026-06-22");

  remove_file(path);
  std::cout << "[PASS] test_save_and_reload_config\n";
}

int main() {
  std::cout << "\n===== ConfigStore Test Suite =====\n\n";

  test_load_legacy_config_keys();
  test_save_and_reload_config();

  std::cout << "\n===== All ConfigStore Tests Passed =====\n\n";
  return 0;
}
