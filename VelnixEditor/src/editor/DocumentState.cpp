#include "editor/DocumentState.h"
#include <glib.h>

bool DocumentState::query_file_state(const std::string &path, gint64 &modifiedTime,
                                     bool &readOnly, FileState &fileState) {
  modifiedTime = 0;
  readOnly = false;
  fileState = FileState::Inaccessible;

  if (path.empty()) {
    return false;
  }

  GFile *file = g_file_new_for_path(path.c_str());
  GError *error = NULL;
  GFileInfo *info = g_file_query_info(file,
                                      G_FILE_ATTRIBUTE_TIME_MODIFIED
                                      "," G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                      G_FILE_QUERY_INFO_NONE, NULL, &error);

  if (!info) {
    if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
      fileState = FileState::Missing;
    }
    if (error) {
      g_error_free(error);
    }
    g_object_unref(file);
    return false;
  }

  modifiedTime = static_cast<gint64>(
      g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED));
  readOnly = !g_file_info_get_attribute_boolean(
      info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
  fileState = FileState::Ok;

  g_object_unref(info);
  g_object_unref(file);
  return true;
}

DocumentStatus DocumentState::compute_document_status(const Document &doc) {
  if (doc.fileState == FileState::Missing) {
    return DocumentStatus::Missing;
  }
  if (doc.fileState == FileState::Inaccessible) {
    return DocumentStatus::Inaccessible;
  }
  if (doc.externallyModified) {
    return DocumentStatus::ExternallyModified;
  }
  if (doc.modified) {
    return DocumentStatus::Modified;
  }
  if (doc.filePath.empty()) {
    return DocumentStatus::Untitled;
  }
  return DocumentStatus::Saved;
}
