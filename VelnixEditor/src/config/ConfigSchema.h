#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include <string>
#include <map>
#include <variant>
#include <vector>
#include <optional>

/**
 * @file ConfigSchema.h
 * 
 * Centralized configuration schema definition for Velnix Editor.
 * 
 * Purpose:
 * - Provide single source of truth for all configuration items
 * - Eliminate duplication between UI, storage, and retrieval logic
 * - Enable easier addition of new config items with type safety
 * 
 * Structure:
 * Each config item is defined with:
 * - Key (used in config file)
 * - Display name (used in UI)
 * - Type (int, bool, string)
 * - Default value
 * - Optional: legacy key aliases
 * - Optional: validation metadata for numeric/string values
 */

namespace ConfigSchema {

// Configuration value type
using ConfigValue = std::variant<bool, int, std::string>;

// Configuration item metadata
struct ConfigItem {
  std::string key;                        // Key in config file (e.g., "show_line_numbers")
  std::string displayName;                // Display name in UI (e.g., "Show Line Numbers")
  ConfigValue defaultValue;               // Default value
  std::string description;                // Help text for UI
  std::string section;                    // Logical grouping (e.g., "editor", "ui", "file")
  std::vector<std::string> legacyKeys;    // Alternate legacy keys accepted from old config files
  std::vector<std::string> allowedValues; // Allowed string values for enums
  std::optional<int> minValue;            // Minimum allowed value for integers
  std::optional<int> maxValue;            // Maximum allowed value for integers
  std::optional<size_t> maxLength;        // Maximum allowed string length
};

// Define all configuration items
namespace Items {

// UI/Display Section
extern const ConfigItem ShowLineNumbers;
extern const ConfigItem UiLanguage;
extern const ConfigItem LineWrapMode;
extern const ConfigItem FontFamily;
extern const ConfigItem FontSize;
extern const ConfigItem ShowWhitespace;
extern const ConfigItem ShowEolMarkers;
extern const ConfigItem HighlightCurrentLine;
extern const ConfigItem ShowIndentGuides;
extern const ConfigItem LongLineColumn;
extern const ConfigItem ScrollPastLastLine;
extern const ConfigItem HighlightMatchingBraces;
extern const ConfigItem ShowRightMarginBackground;

// Syntax Highlighting Section
extern const ConfigItem DefaultLexer;
extern const ConfigItem SyntaxHighlightingEnabled;

// File/Recent Section
extern const ConfigItem UseRecentFiles;
extern const ConfigItem MaxRecentFiles;
extern const ConfigItem LastFileDialogDir;

// Editor Behavior Section
extern const ConfigItem AutoSave;
extern const ConfigItem AutoSaveInterval;
extern const ConfigItem OverwriteReadonly;
extern const ConfigItem TrimTrailingWhitespaceOnSave;
extern const ConfigItem EnsureFinalNewlineOnSave;
extern const ConfigItem CtrlMouseWheelZoom;
extern const ConfigItem HighlightCurrentColumn;
extern const ConfigItem TabSize;
extern const ConfigItem UseSpacesForTabs;
extern const ConfigItem AutoIndent;

// Search Section
extern const ConfigItem SearchWrapAround;
extern const ConfigItem SearchAllDocuments;

// Config Management Section
extern const ConfigItem ConfigDirectory;

}  // namespace Items

// Registry of all config items for iteration and validation
class Registry {
 public:
  static const Registry& instance();

  // Get all items in a section
  std::vector<const ConfigItem*> get_items_in_section(const std::string& section) const;

  // Get item by key (primary or legacy alias)
  const ConfigItem* get_item_by_key(const std::string& key) const;

  // Get the canonical primary key for a given key or alias
  std::string get_canonical_key(const std::string& key) const;

  // Get all primary keys
  std::vector<std::string> get_all_keys() const;

 private:
  Registry();
  std::map<std::string, const ConfigItem*> items_map;
  std::map<std::string, const ConfigItem*> legacy_items_map;
  std::map<std::string, std::string> canonical_key_map;
};

}  // namespace ConfigSchema

#endif  // CONFIG_SCHEMA_H
