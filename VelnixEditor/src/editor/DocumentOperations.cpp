#include "editor/DocumentOperations.h"
#include "editor/DocumentIO.h"
#include "ui/Localization.h"
#include "editor/SaveNormalization.h"
#include "ui/EditorWindow.h"
#include "ui/EventHandler.h"
#include "config/SettingsManager.h"
#include "config/RecentFilesManager.h"
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

void apply_normalized_text_to_editor(GtkWidget *scintilla,
                                     const std::string &text) {
  if (!scintilla) {
    return;
  }

  const sptr_t originalLength = scintilla_send_message(
      SCINTILLA(scintilla), SCI_GETTEXTLENGTH, 0, 0);
  const sptr_t currentPos = scintilla_send_message(
      SCINTILLA(scintilla), SCI_GETCURRENTPOS, 0, 0);
  const sptr_t anchor = scintilla_send_message(SCINTILLA(scintilla),
                                               SCI_GETANCHOR, 0, 0);
  const sptr_t firstVisibleLine = scintilla_send_message(
      SCINTILLA(scintilla), SCI_GETFIRSTVISIBLELINE, 0, 0);
  const sptr_t xOffset = scintilla_send_message(SCINTILLA(scintilla),
                                                SCI_GETXOFFSET, 0, 0);

  scintilla_send_message(SCINTILLA(scintilla), SCI_BEGINUNDOACTION, 0, 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETRANGE, 0,
                         originalLength);
  scintilla_send_message(SCINTILLA(scintilla), SCI_REPLACETARGET,
                         static_cast<sptr_t>(text.size()),
                         reinterpret_cast<sptr_t>(text.c_str()));
  scintilla_send_message(SCINTILLA(scintilla), SCI_ENDUNDOACTION, 0, 0);

  const sptr_t newLength = scintilla_send_message(SCINTILLA(scintilla),
                                                  SCI_GETTEXTLENGTH, 0, 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEL,
                         std::min(anchor, newLength),
                         std::min(currentPos, newLength));
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETFIRSTVISIBLELINE,
                         firstVisibleLine, 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETXOFFSET, xOffset, 0);
}

}  // namespace

namespace {

constexpr std::uintmax_t kLargeFileThresholdBytes =
    16ULL * 1024ULL * 1024ULL;  // 16MB
constexpr std::size_t kEncodingProbeBytes = 64 * 1024;  // 64KB
constexpr std::size_t kReadChunkBytes = 512 * 1024;     // 512KB
constexpr std::size_t kEolSampleLimitBytes = 1024 * 1024;  // 1MB decoded text

bool read_bytes_from_stream(std::ifstream &input, std::size_t maxBytes,
                            std::string &out) {
  out.clear();
  if (maxBytes == 0) {
    return true;
  }
  std::vector<char> buf(maxBytes);
  input.read(buf.data(), static_cast<std::streamsize>(maxBytes));
  const std::streamsize got = input.gcount();
  if (got <= 0) {
    return false;
  }
  out.assign(buf.data(), static_cast<std::size_t>(got));
  return true;
}

EolMode detect_eol_mode_from_sample(const std::string &sample) {
  if (sample.empty()) {
    return EolMode::Lf;
  }
  return DocumentIO::detect_eol_mode(sample);
}

gchar *convert_bytes_to_utf8_with_fallback(const char *data, gssize len,
                                           const char *fromEncoding,
                                           gsize &bytesWritten) {
  bytesWritten = 0;
  GError *error = NULL;
  gchar *converted = g_convert_with_fallback(
      data, len, "UTF-8", fromEncoding, "\xEF\xBF\xBD", NULL, &bytesWritten,
      &error);
  if (error) {
    g_error_free(error);
  }
  return converted;
}

gchar *convert_ansi_chunk_to_utf8(const char *data, gssize len,
                                  const std::string &preferredEncoding,
                                  gsize &bytesWritten) {
  gchar *converted = convert_bytes_to_utf8_with_fallback(
      data, len, preferredEncoding.c_str(), bytesWritten);
  if (converted) {
    return converted;
  }

  if (preferredEncoding != "WINDOWS-1252") {
    converted = convert_bytes_to_utf8_with_fallback(
        data, len, "WINDOWS-1252", bytesWritten);
    if (converted) {
      return converted;
    }
  }

  if (preferredEncoding != "ISO-8859-1") {
    converted = convert_bytes_to_utf8_with_fallback(
        data, len, "ISO-8859-1", bytesWritten);
  }
  return converted;
}

bool load_large_file_into_scintilla(EditorWindow *window, int index,
                                    const std::string &path,
                                    FileEncoding &encodingOut, bool &useBomOut,
                                    EolMode &eolOut, std::string &errorOut) {
  encodingOut = FileEncoding::Utf8;
  useBomOut = false;
  eolOut = EolMode::Lf;
  errorOut.clear();

  const EditorWindow::DocumentEditorState editorState =
      window->getDocumentEditorState(index);
  if (!editorState.scintilla) {
    errorOut = "No editor widget for document.";
    return false;
  }

  std::ifstream probe(path, std::ios::binary);
  if (!probe) {
    errorOut = "Failed to open file for probing: " + std::string(std::strerror(errno));
    return false;
  }

  std::string probeBytes;
  if (!read_bytes_from_stream(probe, kEncodingProbeBytes, probeBytes)) {
    // Empty file is still a valid open.
    probeBytes.clear();
  }

  std::size_t skipBytes = 0;
  if (probeBytes.size() >= 3 &&
      static_cast<unsigned char>(probeBytes[0]) == 0xEF &&
      static_cast<unsigned char>(probeBytes[1]) == 0xBB &&
      static_cast<unsigned char>(probeBytes[2]) == 0xBF) {
    encodingOut = FileEncoding::Utf8;
    useBomOut = true;
    skipBytes = 3;
  } else if (probeBytes.size() >= 2 &&
             static_cast<unsigned char>(probeBytes[0]) == 0xFF &&
             static_cast<unsigned char>(probeBytes[1]) == 0xFE) {
    encodingOut = FileEncoding::Utf16LE;
    useBomOut = true;
    skipBytes = 2;
  } else if (probeBytes.size() >= 2 &&
             static_cast<unsigned char>(probeBytes[0]) == 0xFE &&
             static_cast<unsigned char>(probeBytes[1]) == 0xFF) {
    encodingOut = FileEncoding::Utf16BE;
    useBomOut = true;
    skipBytes = 2;
  } else if (!probeBytes.empty() &&
             g_utf8_validate(probeBytes.data(),
                             static_cast<gssize>(probeBytes.size()), NULL)) {
    encodingOut = FileEncoding::Utf8;
  } else {
    encodingOut = FileEncoding::Ansi;
  }

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    errorOut = "Failed to open file for reading: " + std::string(std::strerror(errno));
    return false;
  }
  if (skipBytes > 0) {
    input.seekg(static_cast<std::streamoff>(skipBytes), std::ios::beg);
  }

  // For very large files, avoid building a giant undo stack.
  scintilla_send_message(SCINTILLA(editorState.scintilla),
                         SCI_SETUNDOCOLLECTION, 0, 0);
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_EMPTYUNDOBUFFER,
                         0, 0);
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETTEXT, 0,
                         reinterpret_cast<sptr_t>(""));

  std::vector<char> chunk(kReadChunkBytes);
  std::string eolSample;
  eolSample.reserve(std::min<std::size_t>(kEolSampleLimitBytes, 256 * 1024));

  std::string utf16Carry;
  while (input) {
    input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    const std::streamsize got = input.gcount();
    if (got <= 0) {
      break;
    }

    if (encodingOut == FileEncoding::Utf8) {
      scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_APPENDTEXT,
                             static_cast<uptr_t>(got),
                             reinterpret_cast<sptr_t>(chunk.data()));
      if (eolSample.size() < kEolSampleLimitBytes) {
        const std::size_t need =
            std::min<std::size_t>(kEolSampleLimitBytes - eolSample.size(),
                                  static_cast<std::size_t>(got));
        eolSample.append(chunk.data(), need);
      }
      continue;
    }

    if (encodingOut == FileEncoding::Ansi) {
      gsize bytesWritten = 0;
      const std::string ansiEncoding = DocumentIO::ansi_encoding_name();
      gchar *converted = convert_ansi_chunk_to_utf8(
          chunk.data(), static_cast<gssize>(got), ansiEncoding, bytesWritten);
      if (!converted) {
        errorOut = "ANSI decode failed (fromEncoding=" + ansiEncoding +
                   ", fallback=WINDOWS-1252, ISO-8859-1).";
        return false;
      }
      scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_APPENDTEXT,
                             static_cast<uptr_t>(bytesWritten),
                             reinterpret_cast<sptr_t>(converted));
      if (eolSample.size() < kEolSampleLimitBytes) {
        const std::size_t need =
            std::min<std::size_t>(kEolSampleLimitBytes - eolSample.size(),
                                  static_cast<std::size_t>(bytesWritten));
        eolSample.append(converted, need);
      }
      g_free(converted);
      continue;
    }

    // UTF-16 streaming decode (keep byte alignment).
    const char *sourceEncoding =
        (encodingOut == FileEncoding::Utf16LE) ? "UTF-16LE" : "UTF-16BE";
    std::string utf16Bytes;
    if (!utf16Carry.empty()) {
      utf16Bytes.swap(utf16Carry);
    }
    utf16Bytes.append(chunk.data(), static_cast<std::size_t>(got));
    if ((utf16Bytes.size() % 2) != 0) {
      utf16Carry.assign(utf16Bytes.data() + (utf16Bytes.size() - 1), 1);
      utf16Bytes.resize(utf16Bytes.size() - 1);
    }
    if (utf16Bytes.empty()) {
      continue;
    }

    // Avoid splitting UTF-16 surrogate pairs across chunks.
    if (utf16Bytes.size() >= 2) {
      const unsigned char b0 =
          static_cast<unsigned char>(utf16Bytes[utf16Bytes.size() - 2]);
      const unsigned char b1 =
          static_cast<unsigned char>(utf16Bytes[utf16Bytes.size() - 1]);
      const std::uint16_t lastUnit =
          (encodingOut == FileEncoding::Utf16LE)
              ? static_cast<std::uint16_t>(b0 | (static_cast<std::uint16_t>(b1) << 8))
              : static_cast<std::uint16_t>((static_cast<std::uint16_t>(b0) << 8) | b1);
      if (lastUnit >= 0xD800 && lastUnit <= 0xDBFF) {
        utf16Carry.assign(utf16Bytes.data() + (utf16Bytes.size() - 2), 2);
        utf16Bytes.resize(utf16Bytes.size() - 2);
        if (utf16Bytes.empty()) {
          continue;
        }
      }
    }

    gsize bytesWritten = 0;
    GError *error = NULL;
    gchar *converted = g_convert_with_fallback(
        utf16Bytes.data(), static_cast<gssize>(utf16Bytes.size()), "UTF-8",
        sourceEncoding, "\xEF\xBF\xBD", NULL, &bytesWritten, &error);
    if (!converted) {
      errorOut = std::string("UTF-16 decode failed (") + sourceEncoding +
                 "): " + (error ? error->message : "(unknown)");
      if (error) g_error_free(error);
      return false;
    }
    scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_APPENDTEXT,
                           static_cast<uptr_t>(bytesWritten),
                           reinterpret_cast<sptr_t>(converted));
    if (eolSample.size() < kEolSampleLimitBytes) {
      const std::size_t need =
          std::min<std::size_t>(kEolSampleLimitBytes - eolSample.size(),
                                static_cast<std::size_t>(bytesWritten));
      eolSample.append(converted, need);
    }
    g_free(converted);
  }

  if (!utf16Carry.empty()) {
    // Remaining bytes could be an incomplete UTF-16 unit/pair; treat as decode error.
    errorOut = "UTF-16 decode ended with incomplete code unit/pair at file end.";
    return false;
  }

  // Re-enable undo collection for subsequent edits (keeps load fast, but
  // editing afterwards behaves normally).
  scintilla_send_message(SCINTILLA(editorState.scintilla),
                         SCI_SETUNDOCOLLECTION, 1, 0);

  eolOut = detect_eol_mode_from_sample(eolSample);
  return true;
}

}  // namespace

DocumentOperations::DocumentOperations(EditorWindow *editorWindow)
    : editorWindow(editorWindow), config(nullptr), testMode(false) {}

DocumentOperations::DocumentOperations(IDocumentOperationsConfig *config)
    : editorWindow(nullptr), config(config), testMode(true) {}

int DocumentOperations::createNewDocument(const std::string &title) {
  Document doc;
  doc.scintilla = scintilla_new();
  doc.tabTextLabel = nullptr;
  doc.closeButton = nullptr;
  doc.bufferId = this->editorWindow->allocateNextBufferId();
  doc.filePath = "";
  doc.title = title;
  doc.modified = false;
  doc.encoding = FileEncoding::Utf8;
  doc.useBom = false;
  doc.eolMode = EolMode::Lf;
  doc.lexerName = this->editorWindow->getDefaultLexerName().empty()
                      ? "null"
                      : this->editorWindow->getDefaultLexerName();
  doc.readOnly = false;
  doc.externallyModified = false;
  doc.lastModifiedTime = 0;
  doc.fileState = FileState::Ok;
  doc.status = DocumentStatus::Untitled;

  // Create tab label with close button
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget *label = gtk_label_new(title.c_str());
  GtkWidget *closeButton = gtk_button_new();
  GtkWidget *closeImage =
      gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
  gtk_button_set_image(GTK_BUTTON(closeButton), closeImage);
  gtk_button_set_relief(GTK_BUTTON(closeButton), GTK_RELIEF_NONE);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), closeButton, FALSE, FALSE, 0);

  doc.tabLabel = hbox;
  doc.tabTextLabel = label;
  doc.closeButton = closeButton;
  EventHandler::enable_file_drop_target(hbox, this->editorWindow);
  EventHandler::enable_file_drop_target(label, this->editorWindow);

  // Connect close button signal
  g_signal_connect(closeButton, "clicked", G_CALLBACK(EventHandler::on_tab_close_clicked),
                   this->editorWindow);
  g_signal_connect(doc.scintilla, SCINTILLA_NOTIFY,
                   G_CALLBACK(EventHandler::on_scintilla_notify), this->editorWindow);
  g_signal_connect(doc.scintilla, "button-press-event",
                   G_CALLBACK(EventHandler::on_scintilla_button_press_event),
                   this->editorWindow);
  g_signal_connect(doc.scintilla, "button-release-event",
                   G_CALLBACK(EventHandler::on_scintilla_button_release_event),
                   this->editorWindow);
  g_signal_connect(doc.scintilla, "motion-notify-event",
                   G_CALLBACK(EventHandler::on_scintilla_motion_notify_event),
                   this->editorWindow);
  g_signal_connect(doc.scintilla, "scroll-event",
                   G_CALLBACK(EventHandler::on_scintilla_scroll_event), this->editorWindow);
  gtk_widget_add_events(doc.scintilla, GDK_BUTTON_PRESS_MASK |
                                         GDK_BUTTON_RELEASE_MASK |
                                         GDK_POINTER_MOTION_MASK |
                                         GDK_SCROLL_MASK);
  EventHandler::enable_file_drop_target(doc.scintilla, this->editorWindow);

  scintilla_send_message(SCINTILLA(doc.scintilla), SCI_SETCODEPAGE, SC_CP_UTF8,
                         0);
  scintilla_send_message(SCINTILLA(doc.scintilla), SCI_SETWRAPMODE,
                         this->editorWindow->getLineWrapMode() == "window"
                             ? SC_WRAP_CHAR
                             : (this->editorWindow->isWordWrapEnabled()
                                    ? SC_WRAP_WORD
                                    : SC_WRAP_NONE),
                         0);
  this->editorWindow->applyEditorSettingsToDocument(doc.scintilla);

  // Add to notebook
  this->editorWindow->appendNotebookPage(doc.scintilla, hbox);
  this->editorWindow->setNotebookPageReorderable(doc.scintilla, true);

  // Widgets added after the main window is shown need to be made visible
  // explicitly.
  gtk_widget_show_all(hbox);
  gtk_widget_show(doc.scintilla);
  this->editorWindow->showNotebook();

  int eolMode = DocumentIO::scintilla_eol_mode(doc.eolMode);
  scintilla_send_message(SCINTILLA(doc.scintilla), SCI_SETEOLMODE, eolMode, 0);
  scintilla_send_message(SCINTILLA(doc.scintilla), SCI_CONVERTEOLS, eolMode, 0);
  int newIndex = this->editorWindow->addDocument(std::move(doc));
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(newIndex);
  if (!editorState.scintilla) {
    return -1;
  }
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETSAVEPOINT, 0,
                         0);

  // Switch to new document
  if (newIndex >= 0 && newIndex < this->editorWindow->getDocumentCount()) {
    this->editorWindow->activateDocument(newIndex);
    this->editorWindow->applyCurrentSyntaxHighlighting();
  }

  return newIndex;
}

void DocumentOperations::closeDocument(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return;
  }

  // Check if document is modified
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (editorState.modified) {
    GtkWidget *dialog = gtk_message_dialog_new(
        this->editorWindow->getDialogParentWindow(), GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE, "%s", Localization::text("prompt.unsaved_changes"));
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog), "%s",
        Localization::text("prompt.save_before_closing"));
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
                           Localization::text("dialog.dont_save"), GTK_RESPONSE_NO,
                           Localization::text("dialog.save"),
                           GTK_RESPONSE_YES, NULL);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
      if (!saveDocument(index)) {
        return; // Save failed, don't close
      }
    } else if (response == GTK_RESPONSE_CANCEL) {
      return; // Don't close
    }
  }

  // Remove from notebook
  this->editorWindow->removeNotebookPage(index);

  // Remove from documents vector
  this->editorWindow->removeDocument(index);

  // Update current document index
  if (this->editorWindow->getDocumentCount() == 0) {
    this->editorWindow->setCurrentDocumentIndex(-1);
    createNewDocument(); // Create a new empty document
  } else {
    int currentDocumentIndex = this->editorWindow->getCurrentDocumentIndex();
    if (currentDocumentIndex >= index) {
      currentDocumentIndex = std::max(0, currentDocumentIndex - 1);
    }
    if (currentDocumentIndex >= 0 &&
        currentDocumentIndex < this->editorWindow->getDocumentCount()) {
      this->editorWindow->activateDocument(currentDocumentIndex);
    }
  }
}

bool DocumentOperations::saveDocument(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return false;
  }
  bool saved;

  const EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!fileState.hasPath()) {
    saved = saveDocumentAs(index);
  } else {
    saved = saveDocumentToPath(index, fileState.path);
  }

  if (saved) {
    this->editorWindow->setDocumentModifiedFlag(index, false);
    this->editorWindow->refreshDocumentState(index, false);
    this->editorWindow->addRecentFile(
        this->editorWindow->getDocumentFileStateSnapshot(index).path);
  }

  return saved;
}

bool DocumentOperations::saveDocumentAs(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return false;
  }

  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Save File", this->editorWindow->getDialogParentWindow(),
      GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel",
      GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                 TRUE);

  const EditorWindow::DocumentFileState currentFileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (currentFileState.hasPath()) {
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                  currentFileState.path.c_str());
  } else if (!this->editorWindow->getLastFileDialogDir().empty()) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        this->editorWindow->getLastFileDialogDir().c_str());
  }

  bool saved = false;
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (filename) {
      EditorWindow::DocumentFileState fileState =
          this->editorWindow->getDocumentFileStateSnapshot(index);
      const std::string previousPath = fileState.path;
      std::string chosenPath = DocumentIO::normalize_path(filename);
      fileState.path = chosenPath;
      this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
      size_t lastSlash = chosenPath.find_last_of("/\\");
      if (lastSlash != std::string::npos) {
        this->editorWindow->setLastFileDialogDir(chosenPath.substr(0, lastSlash));
      }
      saved = saveDocumentToPath(index, chosenPath);
      if (!saved) {
        fileState.path = previousPath;
        this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
      }
      g_free(filename);
    }
  }
  gtk_widget_destroy(dialog);

  this->editorWindow->refreshDocumentState(index, false);
  return saved;
}

bool DocumentOperations::saveDocumentToPath(int index, const std::string &path) {
  if (testMode) {
    // In test mode, we don't have documents vector, so we need to handle differently
    // For now, assume the path is directly used
    std::string normalizedPath = DocumentIO::normalize_path(path);
    std::string rawBytes = SaveNormalization::normalize_text_for_save(
        getDocumentText(index), config->getTrimTrailingWhitespaceOnSave(),
        config->getEnsureFinalNewlineOnSave()); // Assume index 0 for test

    if (config->getOverwriteReadonly()) {
      if (access(normalizedPath.c_str(), F_OK) == 0 && access(normalizedPath.c_str(), W_OK) != 0) {
        struct stat st;
        if (stat(normalizedPath.c_str(), &st) != 0) {
          return false;
        }
        mode_t writableMode = st.st_mode | S_IWUSR;
        if (chmod(normalizedPath.c_str(), writableMode) != 0) {
          return false;
        }
      }
    }

    std::ofstream output(normalizedPath, std::ios::binary);
    if (!output) {
      return false;
    }

    output.write(rawBytes.data(), static_cast<std::streamsize>(rawBytes.size()));
    return output.good();
  }

  Document *doc = this->editorWindow->getDocument(index);
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (!doc || !editorState.scintilla) {
    return false;
  }

  std::string normalizedPath = DocumentIO::normalize_path(path);
  std::string rawBytes;
  const std::string originalText = getDocumentText(index);
  EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  const std::string documentText = SaveNormalization::normalize_text_for_save(
      originalText, editorWindow->shouldTrimTrailingWhitespaceOnSave(),
      editorWindow->shouldEnsureFinalNewlineOnSave());
  if (!DocumentIO::encode_document_text(*doc, documentText, rawBytes)) {
    return false;
  }

  if (editorWindow->shouldOverwriteReadonly()) {
    if (access(normalizedPath.c_str(), F_OK) == 0 && access(normalizedPath.c_str(), W_OK) != 0) {
      struct stat st;
      if (stat(normalizedPath.c_str(), &st) != 0) {
        return false;
      }
      mode_t writableMode = st.st_mode | S_IWUSR;
      if (chmod(normalizedPath.c_str(), writableMode) != 0) {
        return false;
      }
    }
  }

  std::ofstream output(normalizedPath, std::ios::binary);
  if (!output) {
    return false;
  }

  output.write(rawBytes.data(), static_cast<std::streamsize>(rawBytes.size()));
  if (output.good()) {
    if (documentText != originalText) {
      apply_normalized_text_to_editor(editorState.scintilla, documentText);
      this->editorWindow->applyDocumentEolMode(index);
    }
    fileState.path = normalizedPath;
    fileState.externallyModified = false;
    this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
    this->editorWindow->refreshDocumentState(index, true);
    this->editorWindow->markDocumentSaved(index);
    this->editorWindow->refreshDocumentState(index, false);
    return true;
  }
  return false;
}

bool DocumentOperations::loadDocumentFromPath(int index, const std::string &path) {
  set_last_error("");
  if (!this->editorWindow->hasDocument(index)) {
    set_last_error("Target document index is invalid.");
    return false;
  }

  std::string normalizedPath = DocumentIO::normalize_path(path);
  std::error_code ec;
  const std::uintmax_t fileSize =
      std::filesystem::file_size(normalizedPath, ec);
  const bool treatAsLargeFile = (!ec && fileSize >= kLargeFileThresholdBytes);
  if (ec) {
    set_last_error("Failed to stat file size: " + ec.message());
  }

  std::string text;
  FileEncoding encoding = FileEncoding::Utf8;
  bool useBom = false;
  EolMode eolMode = EolMode::Lf;

  if (treatAsLargeFile) {
    std::string errorDetail;
    if (!load_large_file_into_scintilla(this->editorWindow, index, normalizedPath,
                                        encoding, useBom, eolMode, errorDetail)) {
      set_last_error(errorDetail.empty() ? "Large file load failed." : errorDetail);
      return false;
    }
  } else {
    std::ifstream input(normalizedPath, std::ios::binary);
    if (!input) {
      set_last_error("Failed to open file: " + std::string(std::strerror(errno)));
      return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (!DocumentIO::decode_file_content(buffer.str(), text, encoding, useBom)) {
      set_last_error("Decode failed (DocumentIO::decode_file_content returned false).");
      return false;
    }

    eolMode = DocumentIO::detect_eol_mode(text);
  }

  EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  fileState.eolMode = eolMode;
  fileState.path = normalizedPath;
  fileState.encoding = encoding;
  fileState.useBom = useBom;
  fileState.externallyModified = false;
  fileState.lastModifiedTime = 0;
  this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
  if (!treatAsLargeFile) {
    setDocumentText(index, DocumentIO::convert_eol_text(text, eolMode));
  } else {
    // Large file path already populated Scintilla; just apply EOL mode state.
    this->editorWindow->applyDocumentEolMode(index);
  }
  this->editorWindow->applyDocumentEolMode(index);
  this->editorWindow->refreshDocumentState(index, true);
  this->editorWindow->markDocumentSaved(index);
  this->editorWindow->refreshDocumentState(index, false);
  if (!treatAsLargeFile) {
    this->editorWindow->setLexerByFileExtension(normalizedPath);
  } else {
    // Avoid expensive lexer work on huge files by default.
    this->editorWindow->setLexer("null");
  }
  return true;
}

bool DocumentOperations::reloadDocumentFromDisk(int index) {
  set_last_error("");
  if (!this->editorWindow->hasDocument(index)) {
    set_last_error("Target document index is invalid.");
    return false;
  }

  const EditorWindow::DocumentFileState currentFileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!currentFileState.hasPath()) {
    return false;
  }

  std::error_code ec;
  const std::uintmax_t fileSize =
      std::filesystem::file_size(currentFileState.path, ec);
  const bool treatAsLargeFile = (!ec && fileSize >= kLargeFileThresholdBytes);
  if (ec) {
    set_last_error("Failed to stat file size: " + ec.message());
  }

  std::string text;
  FileEncoding encoding = FileEncoding::Utf8;
  bool useBom = false;
  EolMode eolMode = EolMode::Lf;

  if (treatAsLargeFile) {
    std::string errorDetail;
    if (!load_large_file_into_scintilla(this->editorWindow, index,
                                        currentFileState.path, encoding, useBom,
                                        eolMode, errorDetail)) {
      set_last_error(errorDetail.empty() ? "Large file reload failed." : errorDetail);
      return false;
    }
  } else {
    std::ifstream input(currentFileState.path, std::ios::binary);
    if (!input) {
      set_last_error("Failed to open file: " + std::string(std::strerror(errno)));
      return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (!DocumentIO::decode_file_content(buffer.str(), text, encoding, useBom)) {
      set_last_error("Decode failed (DocumentIO::decode_file_content returned false).");
      return false;
    }

    eolMode = DocumentIO::detect_eol_mode(text);
  }

  EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  fileState.encoding = encoding;
  fileState.useBom = useBom;
  fileState.eolMode = eolMode;
  fileState.externallyModified = false;
  this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
  if (!treatAsLargeFile) {
    setDocumentText(index, DocumentIO::convert_eol_text(text, eolMode));
  } else {
    this->editorWindow->applyDocumentEolMode(index);
  }
  this->editorWindow->applyDocumentEolMode(index);
  this->editorWindow->refreshDocumentState(index, true);
  this->editorWindow->markDocumentSaved(index);
  this->editorWindow->refreshDocumentState(index, false);
  return true;
}

std::string DocumentOperations::getDocumentText(int index) {
  if (testMode) {
    return testDocumentText;
  }

  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (!editorState.scintilla) {
    return "";
  }

  int length = static_cast<int>(scintilla_send_message(
      SCINTILLA(editorState.scintilla), SCI_GETTEXTLENGTH, 0, 0));
  std::vector<char> buffer(length + 1);
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_GETTEXT,
                         length + 1,
                         reinterpret_cast<sptr_t>(buffer.data()));
  return std::string(buffer.data(), length);
}

void DocumentOperations::setDocumentText(int index, const std::string &text) {
  if (testMode) {
    testDocumentText = text;
    return;
  }

  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (!editorState.scintilla) {
    return;
  }

  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETTEXT, 0,
                         reinterpret_cast<sptr_t>(text.c_str()));
}
