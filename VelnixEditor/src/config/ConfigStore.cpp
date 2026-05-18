#include "config/ConfigStore.h"
#include "core/Logger.h"
#include <algorithm>
#include <sstream>
#include <iostream>

ConfigStore::ConfigStore(const std::string& configFilePath)
    : configFilePath(configFilePath) {}

bool ConfigStore::load() {
  std::ifstream file(configFilePath);
  if (!file) {
    // File doesn't exist, will use defaults
    lastError = "Config file not found";
    Logger::warning("Configuration file '%s' not found, using defaults.", configFilePath.c_str());
    return false;
  }

  clear();
  std::string line;
  int lineNum = 0;

  while (std::getline(file, line)) {
    lineNum++;
    std::string key, value;

    if (!parse_line(line, key, value)) {
      continue;  // Skip invalid lines
    }

    // Validate key against schema
    const auto* item = ConfigSchema::Registry::instance().get_item_by_key(key);
    if (!item) {
      // Unknown key, skip but don't error
      continue;
    }

    // Parse and store value
    try {
      const std::string canonicalKey =
          ConfigSchema::Registry::instance().get_canonical_key(key);
      values[canonicalKey] = parse_value(canonicalKey, value);
    } catch (const std::exception& e) {
      lastError = "Parse error on line " + std::to_string(lineNum) + ": " +
                  std::string(e.what());
      Logger::warning("Configuration parse error in '%s' at line %d: %s",
                      configFilePath.c_str(), lineNum, e.what());
      continue;
    }
  }

  return true;
}

bool ConfigStore::save() const {
  std::ofstream file(configFilePath, std::ios::trunc);
  if (!file) {
    lastError = "Failed to open config file for writing";
    Logger::error("Unable to open configuration file '%s' for writing.",
                  configFilePath.c_str());
    return false;
  }

  // Write header comment
  file << "# Velnix Editor Configuration\n";
  file << "# Do not edit manually unless you know what you're doing\n\n";

  // Write organized by sections
  const auto& registry = ConfigSchema::Registry::instance();
  std::vector<std::string> sections;

  // Get unique sections
  for (const auto& key : registry.get_all_keys()) {
    const auto* item = registry.get_item_by_key(key);
    if (item && std::find(sections.begin(), sections.end(), item->section) ==
                    sections.end()) {
      sections.push_back(item->section);
    }
  }

  // Write each section
  for (const auto& section : sections) {
    auto items = registry.get_items_in_section(section);
    if (items.empty()) continue;

    file << "# " << section << " settings\n";

    for (const auto* item : items) {
      auto it = values.find(item->key);

      // Use stored value or default
      ConfigSchema::ConfigValue valueToWrite;
      if (it != values.end()) {
        valueToWrite = it->second;
      } else {
        valueToWrite = item->defaultValue;
      }

      // Write key=value
      file << item->key << "=" << std::visit(VariantVisitor(), valueToWrite)
           << "\n";
    }

    file << "\n";
  }

  return file.good();
}

bool ConfigStore::get_bool(const std::string& key) const {
  auto value = get(key);
  if (auto* b = std::get_if<bool>(&value)) {
    return *b;
  }
  return false;
}

int ConfigStore::get_int(const std::string& key) const {
  auto value = get(key);
  if (auto* i = std::get_if<int>(&value)) {
    return *i;
  }
  return 0;
}

std::string ConfigStore::get_string(const std::string& key) const {
  auto value = get(key);
  if (auto* s = std::get_if<std::string>(&value)) {
    return *s;
  }
  return "";
}

void ConfigStore::set_bool(const std::string& key, bool value) {
  set(key, ConfigSchema::ConfigValue(value));
}

void ConfigStore::set_int(const std::string& key, int value) {
  set(key, ConfigSchema::ConfigValue(value));
}

void ConfigStore::set_string(const std::string& key, const std::string& value) {
  set(key, ConfigSchema::ConfigValue(value));
}

ConfigSchema::ConfigValue ConfigStore::get(const std::string& key) const {
  const std::string canonicalKey =
      ConfigSchema::Registry::instance().get_canonical_key(key);
  auto it = values.find(canonicalKey);
  if (it != values.end()) {
    return it->second;
  }

  // Return default from schema
  const auto* item =
      ConfigSchema::Registry::instance().get_item_by_key(canonicalKey);
  if (item) {
    return item->defaultValue;
  }

  // Unknown key with no default
  return false;
}

void ConfigStore::set(const std::string& key,
                      const ConfigSchema::ConfigValue& value) {
  const auto* item = ConfigSchema::Registry::instance().get_item_by_key(key);
  if (!item) {
    throw std::runtime_error("Unknown configuration key: " + key);
  }
  if (!validate_value(item, value)) {
    throw std::runtime_error("Invalid configuration value for key: " + key);
  }
  const std::string canonicalKey =
      ConfigSchema::Registry::instance().get_canonical_key(key);
  values[canonicalKey] = value;
}

bool ConfigStore::has_key(const std::string& key) const {
  const std::string canonicalKey =
      ConfigSchema::Registry::instance().get_canonical_key(key);
  return values.find(canonicalKey) != values.end();
}

std::vector<std::string> ConfigStore::get_all_keys() const {
  std::vector<std::string> keys;
  for (const auto& [key, value] : values) {
    keys.push_back(key);
  }
  return keys;
}

void ConfigStore::clear() {
  values.clear();
  lastError.clear();
}

bool ConfigStore::parse_line(const std::string& line, std::string& key,
                             std::string& value) {
  // Skip empty lines and comments
  if (line.empty() || line[0] == '#') {
    return false;
  }

  size_t equals = line.find('=');
  if (equals == std::string::npos) {
    return false;
  }

  key = line.substr(0, equals);
  value = line.substr(equals + 1);

  return !key.empty();
}

ConfigSchema::ConfigValue ConfigStore::parse_value(const std::string& key,
                                                    const std::string& value) const {
  const auto* item =
      ConfigSchema::Registry::instance().get_item_by_key(key);
  if (!item) {
    throw std::runtime_error("Unknown configuration key: " + key);
  }

  // Parse based on default type
  ConfigSchema::ConfigValue parsedValue;
  if (std::holds_alternative<bool>(item->defaultValue)) {
    parsedValue = value == "1" || value == "true";
  } else if (std::holds_alternative<int>(item->defaultValue)) {
    parsedValue = std::stoi(value);
  } else {
    parsedValue = value;
  }

  if (!validate_value(item, parsedValue)) {
    throw std::runtime_error("Invalid configuration value for key: " + key);
  }

  return parsedValue;
}

bool ConfigStore::validate_value(const ConfigSchema::ConfigItem* item,
                                 const ConfigSchema::ConfigValue& value) const {
  if (std::holds_alternative<bool>(item->defaultValue)) {
    return std::get_if<bool>(&value) != nullptr;
  }

  if (std::holds_alternative<int>(item->defaultValue)) {
    if (auto* intValue = std::get_if<int>(&value)) {
      if (item->minValue && *intValue < *item->minValue) {
        return false;
      }
      if (item->maxValue && *intValue > *item->maxValue) {
        return false;
      }
      return true;
    }
    return false;
  }

  if (std::holds_alternative<std::string>(item->defaultValue)) {
    if (auto* stringValue = std::get_if<std::string>(&value)) {
      if (item->maxLength && stringValue->size() > *item->maxLength) {
        return false;
      }
      if (!item->allowedValues.empty() &&
          std::find(item->allowedValues.begin(), item->allowedValues.end(), *stringValue) == item->allowedValues.end()) {
        return false;
      }
      return true;
    }
    return false;
  }

  return false;
}

std::string ConfigStore::get_default_string(
    const ConfigSchema::ConfigValue& val) const {
  return std::visit(VariantVisitor(), val);
}
