#include "core/UpdateChecker.h"

#include <cassert>
#include <iostream>

int main() {
  assert(UpdateChecker::isNewerVersion("0.1.4", "0.1.3"));
  assert(UpdateChecker::isNewerVersion("v1.0.0", "0.9.9"));
  assert(!UpdateChecker::isNewerVersion("0.1.3", "0.1.3"));
  assert(!UpdateChecker::isNewerVersion("0.1.3.0", "0.1.3"));
  assert(!UpdateChecker::isNewerVersion("invalid", "0.1.3"));
  std::cout << "[PASS] update version comparison\n";
  return 0;
}
