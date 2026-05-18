#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>

class DocumentState {
public:
    static bool query_file_state(const std::string &path, gint64 &modifiedTime,
                                 bool &readOnly, FileState &fileState);
    static DocumentStatus compute_document_status(const Document &doc);
};
