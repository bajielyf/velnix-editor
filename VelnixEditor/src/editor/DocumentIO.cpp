#include "editor/DocumentIO.h"
#include <glib.h>
#include <unicode/ucsdet.h>
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

namespace {

bool decoded_text_matches_encoding(const std::string &bytes,
                                   FileEncoding encoding) {
  const char *name = DocumentIO::legacy_encoding_name(encoding);
  GError *error = NULL;
  gsize written = 0;
  gchar *text = g_convert(bytes.data(), bytes.size(), "UTF-8", name, NULL,
                          &written, &error);
  if (error) g_error_free(error);
  if (!text) return false;
  int expected = 0;
  int nonAscii = 0;
  for (const gchar *p = text, *end = text + written; p < end;
       p = g_utf8_next_char(p)) {
    const gunichar ch = g_utf8_get_char(p);
    if (ch < 0x80) continue;
    ++nonAscii;
    if (encoding == FileEncoding::Cp949 &&
        ((ch >= 0xAC00 && ch <= 0xD7AF) ||
         (ch >= 0x1100 && ch <= 0x11FF))) {
      ++expected;
    } else if (encoding == FileEncoding::ShiftJis &&
               ((ch >= 0x3040 && ch <= 0x30FF) ||
                (ch >= 0x3400 && ch <= 0x9FFF))) {
      ++expected;
    } else if ((encoding == FileEncoding::Big5 ||
                encoding == FileEncoding::Gb18030) &&
               ((ch >= 0x3400 && ch <= 0x9FFF) ||
                (ch >= 0xF900 && ch <= 0xFAFF))) {
      ++expected;
    } else if (encoding != FileEncoding::Cp949 &&
               encoding != FileEncoding::ShiftJis &&
               encoding != FileEncoding::Big5 &&
               encoding != FileEncoding::Gb18030) {
      ++expected;
    }
  }
  g_free(text);
  return expected > 0 && expected * 2 >= nonAscii;
}

}  // namespace

bool DocumentIO::detect_legacy_encoding(const std::string &bytes,
                                        FileEncoding &encoding) {
  if (bytes.empty()) {
    return false;
  }

  // ICU uses statistical language models and is substantially better than
  // byte-validity checks at separating overlapping East Asian encodings.
  UErrorCode status = U_ZERO_ERROR;
  UCharsetDetector *detector = ucsdet_open(&status);
  FileEncoding icuFallback = FileEncoding::Ansi;
  if (U_SUCCESS(status)) {
    ucsdet_setText(detector, bytes.data(), static_cast<int32_t>(bytes.size()),
                   &status);
    int32_t matchCount = 0;
    const UCharsetMatch **matches = ucsdet_detectAll(detector, &matchCount,
                                                     &status);
    for (int32_t i = 0; U_SUCCESS(status) && i < matchCount; ++i) {
      const int32_t confidence = ucsdet_getConfidence(matches[i], &status);
      const char *name = ucsdet_getName(matches[i], &status);
      if (U_FAILURE(status) || !name || confidence < 25) break;
      FileEncoding detected = FileEncoding::Ansi;
      if (g_ascii_strcasecmp(name, "Big5") == 0) detected = FileEncoding::Big5;
      else if (g_ascii_strcasecmp(name, "GB18030") == 0 ||
               g_ascii_strcasecmp(name, "GBK") == 0)
        detected = FileEncoding::Gb18030;
      else if (g_ascii_strcasecmp(name, "Shift_JIS") == 0 ||
               g_ascii_strcasecmp(name, "windows-31j") == 0 ||
               g_ascii_strcasecmp(name, "ibm-943") == 0)
        detected = FileEncoding::ShiftJis;
      else if (g_ascii_strcasecmp(name, "EUC-KR") == 0 ||
               g_ascii_strcasecmp(name, "windows-949") == 0 ||
               g_ascii_strcasecmp(name, "ibm-949") == 0 ||
               g_ascii_strcasecmp(name, "KS_C_5601-1987") == 0)
        detected = FileEncoding::Cp949;
      else if (g_ascii_strcasecmp(name, "windows-1250") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-2") == 0)
        detected = FileEncoding::Windows1250;
      else if (g_ascii_strcasecmp(name, "windows-1251") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-5") == 0)
        detected = FileEncoding::Windows1251;
      else if (g_ascii_strcasecmp(name, "windows-1252") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-1") == 0)
        detected = FileEncoding::Windows1252;
      else if (g_ascii_strcasecmp(name, "windows-1253") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-7") == 0)
        detected = FileEncoding::Windows1253;
      else if (g_ascii_strcasecmp(name, "windows-1254") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-9") == 0)
        detected = FileEncoding::Windows1254;
      else if (g_ascii_strcasecmp(name, "windows-1255") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-8") == 0)
        detected = FileEncoding::Windows1255;
      else if (g_ascii_strcasecmp(name, "windows-1256") == 0 ||
               g_ascii_strcasecmp(name, "ISO-8859-6") == 0)
        detected = FileEncoding::Windows1256;
      else if (g_ascii_strcasecmp(name, "windows-1258") == 0)
        detected = FileEncoding::Windows1258;
      else if (g_ascii_strcasecmp(name, "windows-874") == 0 ||
               g_ascii_strcasecmp(name, "TIS-620") == 0)
        detected = FileEncoding::Windows874;
      if (detected != FileEncoding::Ansi &&
          decoded_text_matches_encoding(bytes, detected)) {
        const bool hasDistinctScript =
            detected == FileEncoding::Windows1251 ||
            detected == FileEncoding::Windows1253 ||
            detected == FileEncoding::Windows1255 ||
            detected == FileEncoding::Windows1256 ||
            detected == FileEncoding::Windows874 ||
            detected == FileEncoding::Cp949 ||
            detected == FileEncoding::ShiftJis ||
            detected == FileEncoding::Gb18030 ||
            detected == FileEncoding::Big5;
        if (hasDistinctScript) {
          encoding = detected;
          ucsdet_close(detector);
          return true;
        }
        // Latin single-byte encodings accept almost every byte sequence. Keep
        // ICU's result only as a fallback so CP949/Shift_JIS text that ICU
        // labels as Windows-1250/1252 can still win by script evidence below.
        if (icuFallback == FileEncoding::Ansi) icuFallback = detected;
      }
    }
    ucsdet_close(detector);
  }

  struct Candidate { FileEncoding encoding; };
  // More distinctive scripts precede overlapping Latin and CJK encodings.
  const Candidate candidates[] = {
      {FileEncoding::Windows1251}, {FileEncoding::Windows1253},
      {FileEncoding::Windows1255}, {FileEncoding::Windows1256},
      {FileEncoding::Windows874},  {FileEncoding::Cp949},
      {FileEncoding::ShiftJis},    {FileEncoding::Gb18030},
      {FileEncoding::Big5},        {FileEncoding::Windows1258},
      {FileEncoding::Windows1254}, {FileEncoding::Windows1250},
      {FileEncoding::Windows1252},
  };

  int bestScore = 0;
  FileEncoding bestEncoding = FileEncoding::Ansi;
  for (const Candidate &candidate : candidates) {
    const char *name = legacy_encoding_name(candidate.encoding);
    gchar *converted = nullptr;
    gsize bytesWritten = 0;
    // A large-file probe may end inside a multibyte sequence.
    for (std::size_t trim = 0; trim <= 3 && trim < bytes.size(); ++trim) {
      GError *error = NULL;
      converted = g_convert(bytes.data(), bytes.size() - trim, "UTF-8", name,
                            NULL, &bytesWritten, &error);
      if (error) g_error_free(error);
      if (converted) break;
    }
    if (!converted) continue;

    int expected = 0;
    int distinctive = 0;
    int unexpected = 0;
    int controls = 0;
    for (const gchar *p = converted, *end = converted + bytesWritten; p < end;
         p = g_utf8_next_char(p)) {
      const gunichar ch = g_utf8_get_char(p);
      if (ch < 0x80) continue;
      if (g_unichar_iscntrl(ch)) { ++controls; continue; }
      const bool cjk = (ch >= 0x3400 && ch <= 0x9FFF) ||
                       (ch >= 0xF900 && ch <= 0xFAFF);
      bool match = false;
      switch (candidate.encoding) {
      case FileEncoding::Windows1251:
        match = ch >= 0x0400 && ch <= 0x052F; break;
      case FileEncoding::Windows1253:
        match = ch >= 0x0370 && ch <= 0x03FF; break;
      case FileEncoding::Windows1255:
        match = ch >= 0x0590 && ch <= 0x05FF; break;
      case FileEncoding::Windows1256:
        match = (ch >= 0x0600 && ch <= 0x06FF) ||
                (ch >= 0x0750 && ch <= 0x077F); break;
      case FileEncoding::Windows874:
        match = ch >= 0x0E00 && ch <= 0x0E7F; break;
      case FileEncoding::Cp949:
        match = (ch >= 0xAC00 && ch <= 0xD7AF) ||
                (ch >= 0x1100 && ch <= 0x11FF); break;
      case FileEncoding::ShiftJis:
        match = cjk || (ch >= 0x3040 && ch <= 0x30FF);
        if (ch >= 0x3040 && ch <= 0x30FF) ++distinctive;
        break;
      case FileEncoding::Gb18030:
      case FileEncoding::Big5:
        match = cjk; break;
      default:
        match = (ch >= 0x00A0 && ch <= 0x024F) ||
                (ch >= 0x0300 && ch <= 0x036F);
        if ((candidate.encoding == FileEncoding::Windows1254 &&
             (ch == 0x011E || ch == 0x011F || ch == 0x0130 || ch == 0x0131 ||
              ch == 0x015E || ch == 0x015F)) ||
            (candidate.encoding == FileEncoding::Windows1258 &&
             ((ch >= 0x1EA0 && ch <= 0x1EF9) ||
              (ch >= 0x0300 && ch <= 0x036F))) ||
            (candidate.encoding == FileEncoding::Windows1250 &&
             (ch == 0x0104 || ch == 0x0105 || ch == 0x010C || ch == 0x010D ||
              ch == 0x0141 || ch == 0x0142 || ch == 0x0158 || ch == 0x0159))) {
          ++distinctive;
        }
        break;
      }
      if (match) ++expected; else ++unexpected;
    }
    g_free(converted);
    if (expected == 0) continue;
    const int score = expected * 10 + distinctive * 25 - unexpected * 8 -
                      controls * 30;
    if (score > bestScore) {
      bestScore = score;
      bestEncoding = candidate.encoding;
    }
  }
  // With only one non-ASCII character, several single-byte encodings can
  // receive the same minimal script score. Prefer ICU's statistical result in
  // that low-evidence case instead of whichever candidate happens to appear
  // first in the table.
  if (icuFallback != FileEncoding::Ansi && bestScore <= 10) {
    encoding = icuFallback;
    return true;
  }
  if (bestScore <= 0) {
    if (icuFallback == FileEncoding::Ansi) return false;
    encoding = icuFallback;
    return true;
  }
  encoding = bestEncoding;
  return true;
}

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

const char *DocumentIO::legacy_encoding_name(FileEncoding encoding) {
  switch (encoding) {
  case FileEncoding::Windows1252: return "WINDOWS-1252";
  case FileEncoding::Windows1250: return "WINDOWS-1250";
  case FileEncoding::Windows1251: return "WINDOWS-1251";
  case FileEncoding::Windows1253: return "WINDOWS-1253";
  case FileEncoding::Windows1254: return "WINDOWS-1254";
  case FileEncoding::Windows1255: return "WINDOWS-1255";
  case FileEncoding::Windows1256: return "WINDOWS-1256";
  case FileEncoding::Windows1258: return "WINDOWS-1258";
  case FileEncoding::Windows874: return "WINDOWS-874";
  case FileEncoding::Gb18030: return "GB18030";
  case FileEncoding::Big5: return "BIG5";
  case FileEncoding::ShiftJis: return "SHIFT_JIS";
  case FileEncoding::Cp949: return "CP949";
  default: return nullptr;
  }
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

  FileEncoding legacyEncoding = FileEncoding::Ansi;
  if (detect_legacy_encoding(rawBytes, legacyEncoding)) {
    GError *error = NULL;
    gsize bytesWritten = 0;
    gchar *converted = g_convert(rawBytes.data(), rawBytes.size(), "UTF-8",
                                 legacy_encoding_name(legacyEncoding), NULL,
                                 &bytesWritten, &error);
    if (converted) {
      encoding = legacyEncoding;
      text.assign(converted, bytesWritten);
      g_free(converted);
      return true;
    }
    if (error) g_error_free(error);
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

bool DocumentIO::decode_file_content_as(const std::string &rawBytes,
                                        FileEncoding encoding, bool useBom,
                                        std::string &text) {
  std::size_t skip = 0;
  const char *sourceEncoding = nullptr;
  if (encoding == FileEncoding::Utf8) {
    if (useBom && rawBytes.size() >= 3 &&
        static_cast<unsigned char>(rawBytes[0]) == 0xEF &&
        static_cast<unsigned char>(rawBytes[1]) == 0xBB &&
        static_cast<unsigned char>(rawBytes[2]) == 0xBF) skip = 3;
    const char *start = rawBytes.data() + skip;
    const gssize length = static_cast<gssize>(rawBytes.size() - skip);
    if (!g_utf8_validate(start, length, NULL)) return false;
    text.assign(start, static_cast<std::size_t>(length));
    return true;
  }
  if (encoding == FileEncoding::Utf16LE) {
    sourceEncoding = "UTF-16LE";
    if (useBom && rawBytes.size() >= 2 &&
        static_cast<unsigned char>(rawBytes[0]) == 0xFF &&
        static_cast<unsigned char>(rawBytes[1]) == 0xFE) skip = 2;
  } else if (encoding == FileEncoding::Utf16BE) {
    sourceEncoding = "UTF-16BE";
    if (useBom && rawBytes.size() >= 2 &&
        static_cast<unsigned char>(rawBytes[0]) == 0xFE &&
        static_cast<unsigned char>(rawBytes[1]) == 0xFF) skip = 2;
  } else if (encoding == FileEncoding::Ansi) {
    const std::string ansi = ansi_encoding_name();
    sourceEncoding = ansi.c_str();
    GError *error = NULL;
    gsize written = 0;
    gchar *converted = g_convert(rawBytes.data(), rawBytes.size(), "UTF-8",
                                 sourceEncoding, NULL, &written, &error);
    if (error) g_error_free(error);
    if (!converted) return false;
    text.assign(converted, written);
    g_free(converted);
    return true;
  } else {
    sourceEncoding = legacy_encoding_name(encoding);
  }
  if (!sourceEncoding) return false;
  GError *error = NULL;
  gsize written = 0;
  gchar *converted = g_convert(rawBytes.data() + skip, rawBytes.size() - skip,
                               "UTF-8", sourceEncoding, NULL, &written, &error);
  if (error) g_error_free(error);
  if (!converted) return false;
  text.assign(converted, written);
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

  if (const char *targetEncoding = legacy_encoding_name(doc.encoding)) {
    GError *error = NULL;
    gsize bytesWritten = 0;
    gchar *converted = g_convert(
        textWithPreferredEol.data(), textWithPreferredEol.size(),
        targetEncoding, "UTF-8", NULL, &bytesWritten, &error);
    if (!converted) {
      if (error) {
        g_error_free(error);
      }
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
