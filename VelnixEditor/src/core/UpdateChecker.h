#pragma once

#include <functional>
#include <string>

class UpdateChecker {
 public:
  struct Result {
    bool success = false;
    std::string latestVersion;
    std::string errorMessage;
  };

  using Callback = std::function<void(Result)>;

  static void checkAsync(Callback callback);
  static bool isNewerVersion(const std::string &candidate,
                             const std::string &current);
};
