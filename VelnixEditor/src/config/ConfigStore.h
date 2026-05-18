#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include "config/ConfigSchema.h"
#include <map>
#include <string>
#include <vector>
#include <fstream>

/**
 * @file ConfigStore.h
 * 
 * Generic configuration storage that uses ConfigSchema for consistency.
 * 
 * Responsibilities:
 * - Load/save configuration from/to file
 * - Get/set configuration values with type checking
 * - Provide defaults for missing values
 * - Handle config file format
 * 
 * Benefits:
 * - All UI code talks to ConfigStore, not directly to EditorWindow
 * - Adding new config items doesn't require modifying I/O logic
 * - Type safety through variant handling
 * - Easy validation and error reporting
 */

class ConfigStore {
 public:
  explicit ConfigStore(const std::string& configFilePath);

  // Load configuration from file
  // Returns true if successful, false if file doesn't exist (will use defaults)
  bool load();

  // Save configuration to file
  // Returns true if successful
  bool save() const;

  // Get typed value by key
  // Returns default if key not found
  bool get_bool(const std::string& key) const;
  int get_int(const std::string& key) const;
  std::string get_string(const std::string& key) const;

  // Set typed value by key
  void set_bool(const std::string& key, bool value);
  void set_int(const std::string& key, int value);
  void set_string(const std::string& key, const std::string& value);

  // Get/set generic ConfigValue
  ConfigSchema::ConfigValue get(const std::string& key) const;
  void set(const std::string& key, const ConfigSchema::ConfigValue& value);

  // Check if key exists in current store
  bool has_key(const std::string& key) const;

  // Get all keys currently in store
  std::vector<std::string> get_all_keys() const;

  // Get last error message
  std::string get_last_error() const { return lastError; }

  // Clear all values (reset to defaults)
  void clear();

 private:
  struct VariantVisitor {
    std::string operator()(bool value) const { return value ? "1" : "0"; }
    std::string operator()(int value) const { return std::to_string(value); }
    std::string operator()(const std::string& value) const { return value; }
  };

  bool parse_line(const std::string& line, std::string& key, std::string& value);
  ConfigSchema::ConfigValue parse_value(const std::string& key,
                                        const std::string& value) const;
  std::string get_default_string(const ConfigSchema::ConfigValue& val) const;
  bool validate_value(const ConfigSchema::ConfigItem* item,
                      const ConfigSchema::ConfigValue& value) const;

  std::string configFilePath;
  std::map<std::string, ConfigSchema::ConfigValue> values;
  mutable std::string lastError;
};

#endif  // CONFIG_STORE_H
