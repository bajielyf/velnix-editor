#include "core/Logger.h"
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace {
static std::FILE *log_file = stderr;
static std::mutex log_mutex;
static bool is_initialized = false;
static bool owns_log_file = false;

const char *level_prefix(Logger::Level level) {
  switch (level) {
    case Logger::Level::Info:
      return "[INFO] ";
    case Logger::Level::Warning:
      return "[WARN] ";
    case Logger::Level::Error:
      return "[ERROR] ";
  }
  return "";
}

void write_log_line(Logger::Level level, const char *message) {
  std::lock_guard<std::mutex> guard(log_mutex);
  std::fprintf(log_file, "%s%s\n", level_prefix(level), message);
  std::fflush(log_file);
}

void close_log_file() {
  std::lock_guard<std::mutex> guard(log_mutex);
  if (owns_log_file && log_file != nullptr) {
    std::fclose(log_file);
  }
  log_file = stderr;
  owns_log_file = false;
}

}  // namespace

namespace Logger {

void init(const std::string &logFilePath) {
  std::lock_guard<std::mutex> guard(log_mutex);
  if (is_initialized) {
    std::fprintf(log_file, "%s%s\n", level_prefix(Level::Warning),
                 "Logger::init() called more than once; ignoring.");
    std::fflush(log_file);
    return;
  }

  if (std::atexit(close_log_file) != 0) {
    std::fprintf(log_file, "%s%s\n", level_prefix(Level::Warning),
                 "Failed to register logger shutdown hook; log file may remain open.");
    std::fflush(log_file);
  }

  if (logFilePath.empty()) {
    log_file = stderr;
    owns_log_file = false;
  } else {
    std::FILE *file = std::fopen(logFilePath.c_str(), "a");
    if (file) {
      log_file = file;
      owns_log_file = true;
    } else {
      log_file = stderr;
      owns_log_file = false;
      std::fprintf(log_file, "%sFailed to open log file '%s'; falling back to stderr.\n",
                   level_prefix(Level::Warning), logFilePath.c_str());
      std::fflush(log_file);
    }
  }

  is_initialized = true;
}

void log(Level level, const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  std::vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write_log_line(level, buffer);
}

void info(const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  std::vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write_log_line(Level::Info, buffer);
}

void warning(const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  std::vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write_log_line(Level::Warning, buffer);
}

void error(const char *format, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, format);
  std::vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write_log_line(Level::Error, buffer);
}

}  // namespace Logger
