#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>

class DocumentIO {
public:
    static std::string basename_from_path(const std::string &path);
    static std::string normalize_path(const std::string &path);
    static int scintilla_eol_mode(EolMode mode);
    static std::string ansi_encoding_name();
    // Returns the iconv/GLib name for a fixed file encoding, or nullptr for
    // encodings whose byte order/name is handled separately.
    static const char *legacy_encoding_name(FileEncoding encoding);
    static bool detect_legacy_encoding(const std::string &bytes,
                                       FileEncoding &encoding);
    static EolMode detect_eol_mode(const std::string &text);
    static std::string convert_eol_text(const std::string &text, EolMode mode);
    static bool decode_file_content(const std::string &rawBytes, std::string &text,
                                   FileEncoding &encoding, bool &useBom);
    static bool decode_file_content_as(const std::string &rawBytes,
                                       FileEncoding encoding, bool useBom,
                                       std::string &text);
    static bool encode_document_text(const Document &doc, const std::string &text,
                                     std::string &rawBytes);
};
