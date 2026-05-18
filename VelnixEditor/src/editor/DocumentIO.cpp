#include "editor/DocumentIO.h"
#include <glib.h>
#include <cstdlib>
#include <string>

namespace {

bool looks_like_utf16_without_bom(const std::string &bytes,
                                 FileEncoding &encodingOut) {
  const std::size_t sampleSize = std::min<std::size_t>(bytes.size(), 4096);
  if (sampleSize < 4) {
    return false;
  }

  std::size_t evenZeros = 0;
  std::size_t oddZeros = 0;
  std::size_t pairs = sampleSize / 2;
  for (std::size_t i = 0; i + 1 < sampleSize; i += 2) {
    if (bytes[i] == '\0') {
      ++evenZeros;
    }
    if (bytes[i + 1] == '\0') {
      ++oddZeros;
    }
  }

  // Heuristic: typical UTF-16 text in ASCII range has lots of zeros in one
  // byte lane. Require a meaningful signal to avoid misdetecting binary files.
  const double evenRatio = pairs > 0 ? static_cast<double>(evenZeros) / pairs : 0.0;
  const double oddRatio = pairs > 0 ? static_cast<double>(oddZeros) / pairs : 0.0;

  if (oddRatio > 0.30 && evenRatio < 0.05) {
    encodingOut = FileEncoding::Utf16LE;
    return true;
  }
  if (evenRatio > 0.30 && oddRatio < 0.05) {
    encodingOut = FileEncoding::Utf16BE;
    return true;
  }
  return false;
}

gchar *convert_to_utf8_with_fallback(const char *data, gssize len,
                                    const char *fromEncoding,
                                    gsize &bytesWritten) {
  bytesWritten = 0;
  GError *error = NULL;
  gchar *converted = g_convert_with_fallback(
      data, len, "UTF-8", fromEncoding, "\xEF\xBF\xBD", NULL, &bytesWritten,
      &error);
  if (!converted) {
    if (error) {
      g_error_free(error);
    }
    return nullptr;
  }
  return converted;
}

gchar *convert_ansi_bytes_to_utf8(const char *data, gssize len,
                                  const std::string &preferredEncoding,
                                  gsize &bytesWritten) {
  gchar *converted =
      convert_to_utf8_with_fallback(data, len, preferredEncoding.c_str(),
                                    bytesWritten);
  if (converted) {
    return converted;
  }

  if (preferredEncoding != "WINDOWS-1252") {
    converted = convert_to_utf8_with_fallback(data, len, "WINDOWS-1252",
                                              bytesWritten);
    if (converted) {
      return converted;
    }
  }

  if (preferredEncoding != "ISO-8859-1") {
    converted = convert_to_utf8_with_fallback(data, len, "ISO-8859-1",
                                              bytesWritten);
  }
  return converted;
}

gchar *convert_utf8_to_ansi_bytes(const char *data, gssize len,
                                  const std::string &preferredEncoding,
                                  gsize &bytesWritten) {
  bytesWritten = 0;
  GError *error = NULL;
  gchar *converted =
      g_convert(data, len, preferredEncoding.c_str(), "UTF-8", NULL,
                &bytesWritten, &error);
  if (converted) {
    return converted;
  }
  if (error) {
    g_error_free(error);
    error = NULL;
  }

  if (preferredEncoding != "WINDOWS-1252") {
    converted = g_convert(data, len, "WINDOWS-1252", "UTF-8", NULL,
                          &bytesWritten, &error);
    if (converted) {
      return converted;
    }
    if (error) {
      g_error_free(error);
      error = NULL;
    }
  }

  if (preferredEncoding != "ISO-8859-1") {
    converted = g_convert(data, len, "ISO-8859-1", "UTF-8", NULL,
                          &bytesWritten, &error);
  }
  if (error) {
    g_error_free(error);
  }
  return converted;
}

}  // namespace

std::string DocumentIO::basename_from_path(const std::string &path) {
  size_t lastSlash = path.find_last_of("/\\");
  return (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
}

std::string DocumentIO::normalize_path(const std::string &path) {
  if (path.empty()) {
    return path;
  }

  gchar *canonical = g_canonicalize_filename(path.c_str(), NULL);
  if (!canonical) {
    return path;
  }

  std::string normalized = canonical;
  g_free(canonical);
  return normalized;
}

int DocumentIO::scintilla_eol_mode(EolMode mode) {
  switch (mode) {
  case EolMode::CrLf:
    return SC_EOL_CRLF;
  case EolMode::Cr:
    return SC_EOL_CR;
  case EolMode::Lf:
  default:
    return SC_EOL_LF;
  }
}

std::string DocumentIO::ansi_encoding_name() {
  const char *overrideEncoding = std::getenv("VELNIX_EDITOR_ANSI_ENCODING");
  if (overrideEncoding && overrideEncoding[0] != '\0') {
    return overrideEncoding;
  }

  const char *charset = nullptr;
  const gboolean localeIsUtf8 = g_get_charset(&charset);
  if (!localeIsUtf8 && charset && charset[0] != '\0') {
    return charset;
  }

  return "WINDOWS-1252";
}

EolMode DocumentIO::detect_eol_mode(const std::string &text) {
  size_t crlfCount = 0;
  size_t lfCount = 0;
  size_t crCount = 0;

  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        ++crlfCount;
        ++i;
      } else {
        ++crCount;
      }
    } else if (text[i] == '\n') {
      ++lfCount;
    }
  }

  if (crlfCount >= lfCount && crlfCount >= crCount && crlfCount > 0) {
    return EolMode::CrLf;
  }
  if (lfCount >= crCount && lfCount > 0) {
    return EolMode::Lf;
  }
  if (crCount > 0) {
    return EolMode::Cr;
  }
  return EolMode::Lf;
}

std::string DocumentIO::convert_eol_text(const std::string &text, EolMode targetMode) {
  std::string normalized;
  normalized.reserve(text.size());

  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r') {
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      normalized.push_back('\n');
    } else {
      normalized.push_back(text[i]);
    }
  }

  if (targetMode == EolMode::Lf) {
    return normalized;
  }

  std::string converted;
  converted.reserve(normalized.size() * (targetMode == EolMode::CrLf ? 2 : 1));
  for (char ch : normalized) {
    if (ch == '\n') {
      if (targetMode == EolMode::CrLf) {
        converted.append("\r\n");
      } else {
        converted.push_back('\r');
      }
    } else {
      converted.push_back(ch);
    }
  }
  return converted;
}

bool DocumentIO::decode_file_content(const std::string &rawBytes,
                                    std::string &text,
                                    FileEncoding &encoding,
                                    bool &useBom) {
  useBom = false;

  if (rawBytes.size() >= 3 && static_cast<unsigned char>(rawBytes[0]) == 0xEF &&
      static_cast<unsigned char>(rawBytes[1]) == 0xBB &&
      static_cast<unsigned char>(rawBytes[2]) == 0xBF) {
    encoding = FileEncoding::Utf8;
    useBom = true;
    text = rawBytes.substr(3);
    return true;
  }

  if (rawBytes.size() >= 2 && static_cast<unsigned char>(rawBytes[0]) == 0xFF &&
      static_cast<unsigned char>(rawBytes[1]) == 0xFE) {
    gsize bytesWritten = 0;
    gchar *converted = convert_to_utf8_with_fallback(
        rawBytes.data() + 2, static_cast<gssize>(rawBytes.size() - 2), "UTF-16LE",
        bytesWritten);
    if (!converted) {
      return false;
    }
    encoding = FileEncoding::Utf16LE;
    useBom = true;
    text.assign(converted, bytesWritten);
    g_free(converted);
    return true;
  }

  if (rawBytes.size() >= 2 && static_cast<unsigned char>(rawBytes[0]) == 0xFE &&
      static_cast<unsigned char>(rawBytes[1]) == 0xFF) {
    gsize bytesWritten = 0;
    gchar *converted = convert_to_utf8_with_fallback(
        rawBytes.data() + 2, static_cast<gssize>(rawBytes.size() - 2), "UTF-16BE",
        bytesWritten);
    if (!converted) {
      return false;
    }
    encoding = FileEncoding::Utf16BE;
    useBom = true;
    text.assign(converted, bytesWritten);
    g_free(converted);
    return true;
  }

  if (g_utf8_validate(rawBytes.data(), rawBytes.size(), NULL)) {
    encoding = FileEncoding::Utf8;
    text = rawBytes;
    return true;
  }

  // UTF-16 without BOM (heuristic).
  FileEncoding utf16Guess = FileEncoding::Utf16LE;
  if (looks_like_utf16_without_bom(rawBytes, utf16Guess)) {
    const char *sourceEncoding =
        (utf16Guess == FileEncoding::Utf16LE) ? "UTF-16LE" : "UTF-16BE";
    gsize bytesWritten = 0;
    gchar *converted = convert_to_utf8_with_fallback(
        rawBytes.data(), static_cast<gssize>(rawBytes.size()), sourceEncoding,
        bytesWritten);
    if (converted) {
      encoding = utf16Guess;
      useBom = false;
      text.assign(converted, bytesWritten);
      g_free(converted);
      return true;
    }
  }

  // ANSI fallback (with replacement on invalid sequences).
  gsize bytesWritten = 0;
  const std::string ansiEncoding = ansi_encoding_name();
  gchar *converted = convert_ansi_bytes_to_utf8(
      rawBytes.data(), static_cast<gssize>(rawBytes.size()), ansiEncoding,
      bytesWritten);
  if (!converted) {
    return false;
  }

  encoding = FileEncoding::Ansi;
  text.assign(converted, bytesWritten);
  g_free(converted);
  return true;
}

bool DocumentIO::encode_document_text(const Document &doc, const std::string &text,
                                      std::string &rawBytes) {
  rawBytes.clear();
  std::string textWithPreferredEol = convert_eol_text(text, doc.eolMode);

  if (doc.encoding == FileEncoding::Utf8) {
    if (doc.useBom) {
      rawBytes.append("\xEF\xBB\xBF", 3);
    }
    rawBytes.append(textWithPreferredEol);
    return true;
  }

  if (doc.encoding == FileEncoding::Ansi) {
    gsize bytesWritten = 0;
    const std::string ansiEncoding = ansi_encoding_name();
    gchar *converted = convert_utf8_to_ansi_bytes(
        textWithPreferredEol.data(),
        static_cast<gssize>(textWithPreferredEol.size()), ansiEncoding,
        bytesWritten);
    if (!converted) {
      return false;
    }

    rawBytes.append(converted, bytesWritten);
    g_free(converted);
    return true;
  }

  const char *targetEncoding =
      (doc.encoding == FileEncoding::Utf16LE) ? "UTF-16LE" : "UTF-16BE";
  GError *error = NULL;
  gsize bytesWritten = 0;
  gchar *converted = g_convert(textWithPreferredEol.data(), textWithPreferredEol.size(),
                                targetEncoding, "UTF-8", NULL, &bytesWritten, &error);
  if (!converted) {
    if (error) {
      g_error_free(error);
    }
    return false;
  }

  if (doc.useBom) {
    if (doc.encoding == FileEncoding::Utf16LE) {
      rawBytes.append("\xFF\xFE", 2);
    } else {
      rawBytes.append("\xFE\xFF", 2);
    }
  }

  rawBytes.append(converted, bytesWritten);
  g_free(converted);
  return true;
}
