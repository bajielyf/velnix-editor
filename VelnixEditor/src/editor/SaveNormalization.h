#pragma once

#include <string>

namespace SaveNormalization {

std::string trim_trailing_whitespace(const std::string &text);
std::string normalize_text_for_save(const std::string &text,
                                    bool trimWhitespace,
                                    bool ensureFinalNewline);

}  // namespace SaveNormalization
