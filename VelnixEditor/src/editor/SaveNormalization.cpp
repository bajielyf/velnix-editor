#include "editor/SaveNormalization.h"

namespace SaveNormalization {

std::string trim_trailing_whitespace(const std::string &text) {
  std::string result;
  result.reserve(text.size());

  size_t lineStart = 0;
  size_t position = 0;

  while (position < text.size()) {
    if (text[position] != '\r' && text[position] != '\n') {
      ++position;
      continue;
    }

    size_t trimEnd = position;
    while (trimEnd > lineStart &&
           (text[trimEnd - 1] == ' ' || text[trimEnd - 1] == '\t')) {
      --trimEnd;
    }

    result.append(text, lineStart, trimEnd - lineStart);

    if (text[position] == '\r' && position + 1 < text.size() &&
        text[position + 1] == '\n') {
      result.append("\r\n");
      position += 2;
    } else {
      result.push_back(text[position]);
      ++position;
    }

    lineStart = position;
  }

  size_t trimEnd = text.size();
  while (trimEnd > lineStart &&
         (text[trimEnd - 1] == ' ' || text[trimEnd - 1] == '\t')) {
    --trimEnd;
  }
  result.append(text, lineStart, trimEnd - lineStart);

  return result;
}

std::string normalize_text_for_save(const std::string &text,
                                    bool trimWhitespace,
                                    bool ensureFinalNewline) {
  std::string normalized = trimWhitespace ? trim_trailing_whitespace(text) : text;
  if (ensureFinalNewline && !normalized.empty() && normalized.back() != '\n' &&
      normalized.back() != '\r') {
    normalized.push_back('\n');
  }
  return normalized;
}

}  // namespace SaveNormalization
