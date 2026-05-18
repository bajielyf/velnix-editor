#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace Logger {

enum class Level {
  Info,
  Warning,
  Error,
};

void init(const std::string &logFilePath = "");
void log(Level level, const char *format, ...);
void info(const char *format, ...);
void warning(const char *format, ...);
void error(const char *format, ...);

}  // namespace Logger

#endif  // LOGGER_H
