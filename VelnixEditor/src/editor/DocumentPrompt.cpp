#include "editor/DocumentPrompt.h"
#include "editor/DocumentIO.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"
#include <gtk/gtk.h>

DocumentPrompt::DocumentPrompt(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

bool DocumentPrompt::promptForMissingFile(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return false;
  }

  const EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);

  if (fileState.fileState != FileState::Missing) {
    return false;
  }

  this->editorWindow->switchToDocument(index);

  GtkWidget *dialog = gtk_message_dialog_new(
      this->editorWindow->getDialogParentWindow(), GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_NONE, "%s", Localization::text("error.missing_file"));
  const std::string secondary =
      std::string(Localization::text("prompt.keep_buffer_prefix")) +
      DocumentIO::basename_from_path(fileState.path) +
      Localization::text("prompt.keep_buffer_suffix");
  gtk_message_dialog_format_secondary_text(
      GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                         Localization::text("dialog.close_buffer"),
                         GTK_RESPONSE_NO,
                         Localization::text("dialog.keep_open"),
                         GTK_RESPONSE_YES, NULL);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  if (response == GTK_RESPONSE_NO) {
    this->editorWindow->closeDocument(index);
  } else {
    EditorWindow::DocumentFileState updatedState = fileState;
    updatedState.fileState = FileState::Missing;
    updatedState.externallyModified = false;
    this->editorWindow->applyDocumentFileStateSnapshot(index, updatedState);
    this->editorWindow->refreshDocumentState(index, false);
  }
  return true;
}

bool DocumentPrompt::promptForInaccessibleFile(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return false;
  }

  const EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);

  if (fileState.fileState != FileState::Inaccessible) {
    return false;
  }

  this->editorWindow->switchToDocument(index);

  GtkWidget *dialog = gtk_message_dialog_new(
      this->editorWindow->getDialogParentWindow(), GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_NONE, "%s", Localization::text("error.inaccessible_file"));
  const std::string secondary =
      std::string(Localization::text("prompt.keep_buffer_prefix")) +
      DocumentIO::basename_from_path(fileState.path) +
      Localization::text("prompt.keep_try_again_suffix");
  gtk_message_dialog_format_secondary_text(
      GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                         Localization::text("dialog.close_buffer"),
                         GTK_RESPONSE_NO,
                         Localization::text("dialog.keep_open"),
                         GTK_RESPONSE_YES, NULL);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  if (response == GTK_RESPONSE_NO) {
    this->editorWindow->closeDocument(index);
  } else {
    this->editorWindow->refreshDocumentState(index, false);
  }
  return true;
}

bool DocumentPrompt::checkExternalModificationForDocument(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return false;
  }

  const EditorWindow::DocumentFileState beforeRefresh =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!beforeRefresh.hasPath()) {
    return false;
  }

  gint64 previousTimestamp = beforeRefresh.lastModifiedTime;
  this->editorWindow->syncDocumentFileState(index);
  const EditorWindow::DocumentFileState afterRefresh =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (afterRefresh.fileState == FileState::Missing) {
    return promptForMissingFile(index);
  }
  if (afterRefresh.fileState == FileState::Inaccessible) {
    return promptForInaccessibleFile(index);
  }
  if (!afterRefresh.externallyModified ||
      afterRefresh.lastModifiedTime == previousTimestamp) {
    return false;
  }

  this->editorWindow->switchToDocument(index);

  GtkWidget *dialog = gtk_message_dialog_new(
      this->editorWindow->getDialogParentWindow(), GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_NONE, "%s", Localization::text("error.changed_on_disk"));
  const std::string secondary =
      std::string(Localization::text("prompt.reload_prefix")) +
      DocumentIO::basename_from_path(afterRefresh.path) +
      Localization::text("prompt.reload_suffix");
  gtk_message_dialog_format_secondary_text(
      GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                         Localization::text("dialog.ignore"), GTK_RESPONSE_NO,
                         Localization::text("dialog.reload"), GTK_RESPONSE_YES,
                         NULL);

  int response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  if (response == GTK_RESPONSE_YES) {
    if (!this->editorWindow->reloadDocumentFromDisk(index)) {
      show_error_dialog(Localization::text("error.unable_reload"),
                        afterRefresh.path);
    }
  } else {
    this->editorWindow->acknowledgeFileState(index);
  }
  return true;
}

void DocumentPrompt::show_error_dialog(const std::string &primary,
                                       const std::string &secondary) {
  GtkWidget *dialog = gtk_message_dialog_new(
      this->editorWindow->getDialogParentWindow(), GTK_DIALOG_MODAL,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE, "%s", primary.c_str());
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           secondary.c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}
