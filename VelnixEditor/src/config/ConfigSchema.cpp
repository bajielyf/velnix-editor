#include "config/ConfigSchema.h"

namespace ConfigSchema {

namespace Items {

// UI/Display Section
const ConfigItem ShowLineNumbers{
    "show_line_numbers",
    "Show Line Numbers",
    true,
    "Display line numbers in the editor margin",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem UiLanguage{
    "ui_language",
    "Interface Language",
    std::string("en"),
    "Language used by the application interface",
    "ui",
    {},
    {"en", "ja", "ko", "es", "pt", "de", "fr", "ar", "yue", "zh-Hans", "zh-Hant"},
    {},
    {},
    {}
};

const ConfigItem LineWrapMode{
    "line_wrap_mode",
    "Line Wrap Mode",
    std::string("off"),
    "Enable automatic line wrapping: off, window, word",
    "ui",
    {},
    {"off", "window", "word"},
    {},
    {},
    {}
};

const ConfigItem FontFamily{
    "font_family",
    "Font Family",
    std::string("Monospace"),
    "Font face for editor content",
    "ui",
    {},
    {},
    {},
    {},
    64
};

const ConfigItem FontSize{
    "font_size",
    "Font Size",
    11,
    "Font size in points (8-72)",
    "ui",
    {},
    {},
    8,
    72,
    {}
};

const ConfigItem ShowWhitespace{
    "show_whitespace",
    "Show Whitespace",
    false,
    "Display spaces and tabs using visible markers",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem ShowEolMarkers{
    "show_eol_markers",
    "Show End-of-Line Markers",
    false,
    "Display end-of-line markers for each line",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem HighlightCurrentLine{
    "highlight_current_line",
    "Highlight Current Line",
    true,
    "Highlight the line containing the caret",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem ShowIndentGuides{
    "show_indent_guides",
    "Show Indent Guides",
    true,
    "Draw vertical indentation guides",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem LongLineColumn{
    "long_line_column",
    "Long Line Column",
    120,
    "Column for the long line reference guide (0 disables it)",
    "ui",
    {},
    {},
    0,
    240,
    {}
};

const ConfigItem ScrollPastLastLine{
    "scroll_past_last_line",
    "Scroll Past Last Line",
    true,
    "Allow scrolling beyond the last line for easier editing near the end of a document",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem HighlightMatchingBraces{
    "highlight_matching_braces",
    "Highlight Matching Braces",
    true,
    "Highlight matching brackets and show mismatches near the caret",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem ShowRightMarginBackground{
    "show_right_margin_background",
    "Highlight Text Beyond Right Margin",
    false,
    "Highlight text beyond the long line column instead of drawing a thin guide line",
    "ui",
    {},
    {},
    {},
    {},
    {}
};

// Syntax Highlighting Section
const ConfigItem DefaultLexer{
    "default_lexer",
    "Default Lexer",
    std::string("null"),
    "Default syntax highlighting lexer for new documents",
    "syntax",
    {},
    {},
    {},
    {},
    64
};

const ConfigItem SyntaxHighlightingEnabled{
    "syntax_highlighting_enabled",
    "Enable Syntax Highlighting",
    true,
    "Enable syntax highlighting for supported file types",
    "syntax",
    {},
    {},
    {},
    {},
    {}
};

// File/Recent Section
const ConfigItem UseRecentFiles{
    "use_recent_files",
    "Show Recent Files",
    true,
    "Display recently opened files in File menu",
    "file",
    {"use_recent"},
    {},
    {},
    {},
    {}
};

const ConfigItem MaxRecentFiles{
    "max_recent_files",
    "Maximum Recent Files",
    5,
    "Number of recent files to keep (1-20)",
    "file",
    {"max_recent"},
    {},
    1,
    20,
    {}
};

const ConfigItem LastFileDialogDir{
    "last_file_dialog_dir",
    "Last File Dialog Directory",
    std::string(""),
    "Last directory used in file open/save dialogs",
    "file",
    {"last_file_dir"},
    {},
    {},
    {},
    1024
};

// Editor Behavior Section
const ConfigItem AutoSave{
    "auto_save",
    "Enable Auto-Save",
    false,
    "Automatically save documents at intervals",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem AutoSaveInterval{
    "auto_save_interval",
    "Auto-Save Interval",
    60,
    "Interval in seconds for auto-save (10-300)",
    "editor",
    {},
    {},
    10,
    300,
    {}
};

const ConfigItem OverwriteReadonly{
    "overwrite_readonly",
    "Overwrite Read-Only Files",
    false,
    "Allow saving to read-only files (requires elevated privileges)",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem TrimTrailingWhitespaceOnSave{
    "trim_trailing_whitespace_on_save",
    "Trim Trailing Whitespace on Save",
    false,
    "Remove trailing spaces and tabs from each line when saving files",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem EnsureFinalNewlineOnSave{
    "ensure_final_newline_on_save",
    "Ensure Final Newline on Save",
    false,
    "Add a newline at the end of the file when saving if one is missing",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem CtrlMouseWheelZoom{
    "ctrl_mouse_wheel_zoom",
    "Enable Ctrl+Mouse Wheel Zoom",
    true,
    "Allow changing the editor zoom level with Ctrl + mouse wheel",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem SmartKeywordHighlighting{
    "smart_keyword_highlighting",
    "Enable Smart Keyword Highlighting",
    false,
    "Highlight matching occurrences when a word or token is selected",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem HighlightCurrentColumn{
    "highlight_current_column",
    "Show Current Column Guide",
    false,
    "Draw a vertical guide line at the current caret column.",
    "editor",
    {},
    {},
    {},
    {},
    {}
};

// Search Section
const ConfigItem SearchWrapAround{
    "search_wrap_around",
    "Search Wrap Around",
    true,
    "Continue searching from beginning when reaching end",
    "search",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem SearchAllDocuments{
    "search_all_documents",
    "Search All Documents",
    false,
    "Search in all open documents by default",
    "search",
    {},
    {},
    {},
    {},
    {}
};

// Config Management Section
const ConfigItem ConfigDirectory{
    "config_directory",
    "Config Directory",
    std::string(""),
    "Custom configuration directory (auto-managed if empty)",
    "config",
    {"config_dir"},
    {},
    {},
    {},
    1024
};

const ConfigItem LastUpdateCheckDate{
    "last_update_check_date",
    "Last Update Check Date",
    std::string(""),
    "Local date of the most recent automatic update check",
    "config",
    {},
    {},
    {},
    {},
    10
};

// Editor Indentation Section
const ConfigItem TabSize{
    "tab_size",
    "Tab Size",
    4,
    "Width of a tab in spaces (2-8)",
    "indentation",
    {},
    {},
    2,
    8,
    {}
};

const ConfigItem UseSpacesForTabs{
    "use_spaces_for_tabs",
    "Use Spaces for Tabs",
    true,
    "Insert spaces instead of tab character when Tab key is pressed",
    "indentation",
    {},
    {},
    {},
    {},
    {}
};

const ConfigItem AutoIndent{
    "auto_indent",
    "Auto Indent",
    true,
    "Automatically indent new lines to match the previous line's indentation",
    "indentation",
    {},
    {},
    {},
    {},
    {}
};

}  // namespace Items

// Registry implementation
Registry::Registry() {
  const std::vector<const ConfigItem*> allItems = {
      &Items::ShowLineNumbers,
      &Items::UiLanguage,
      &Items::LineWrapMode,
      &Items::FontFamily,
      &Items::FontSize,
      &Items::ShowWhitespace,
      &Items::ShowEolMarkers,
      &Items::HighlightCurrentLine,
      &Items::ShowIndentGuides,
      &Items::LongLineColumn,
      &Items::ScrollPastLastLine,
      &Items::HighlightMatchingBraces,
      &Items::ShowRightMarginBackground,
      &Items::DefaultLexer,
      &Items::SyntaxHighlightingEnabled,
      &Items::UseRecentFiles,
      &Items::MaxRecentFiles,
      &Items::LastFileDialogDir,
      &Items::AutoSave,
      &Items::AutoSaveInterval,
      &Items::OverwriteReadonly,
      &Items::TrimTrailingWhitespaceOnSave,
      &Items::EnsureFinalNewlineOnSave,
      &Items::CtrlMouseWheelZoom,
      &Items::SmartKeywordHighlighting,
      &Items::HighlightCurrentColumn,
      &Items::SearchWrapAround,
      &Items::SearchAllDocuments,
      &Items::ConfigDirectory,
      &Items::LastUpdateCheckDate,
      &Items::TabSize,
      &Items::UseSpacesForTabs,
      &Items::AutoIndent,
  };

  for (const auto* item : allItems) {
    items_map[item->key] = item;
    canonical_key_map[item->key] = item->key;
    for (const auto& legacyKey : item->legacyKeys) {
      legacy_items_map[legacyKey] = item;
      canonical_key_map[legacyKey] = item->key;
    }
  }
}

const Registry& Registry::instance() {
  static Registry registry;
  return registry;
}

std::vector<const ConfigItem*> Registry::get_items_in_section(
    const std::string& section) const {
  std::vector<const ConfigItem*> result;
  for (const auto& [key, item] : items_map) {
    if (item->section == section) {
      result.push_back(item);
    }
  }
  return result;
}

const ConfigItem* Registry::get_item_by_key(const std::string& key) const {
  auto it = items_map.find(key);
  if (it != items_map.end()) {
    return it->second;
  }
  auto legacyIt = legacy_items_map.find(key);
  if (legacyIt != legacy_items_map.end()) {
    return legacyIt->second;
  }
  return nullptr;
}

std::string Registry::get_canonical_key(const std::string& key) const {
  auto it = canonical_key_map.find(key);
  if (it != canonical_key_map.end()) {
    return it->second;
  }
  return key;
}

std::vector<std::string> Registry::get_all_keys() const {
  std::vector<std::string> keys;
  for (const auto& [key, item] : items_map) {
    keys.push_back(key);
  }
  return keys;
}

}  // namespace ConfigSchema
