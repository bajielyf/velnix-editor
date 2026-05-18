#include "editor/DocumentRefresh.h"
#include "editor/DocumentIO.h"
#include "editor/DocumentState.h"
#include "ui/EditorWindow.h"
#include <gtk/gtk.h>

DocumentRefresh::DocumentRefresh(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

void DocumentRefresh::refreshDocumentState(int index, bool syncDiskState) {
  if (!this->editorWindow->hasDocument(index)) {
    return;
  }

  if (syncDiskState) {
    syncDocumentFileState(index);
    return;
  }

  this->editorWindow->recomputeDocumentStatus(index);
}

void DocumentRefresh::refreshAllDocumentStates(bool syncDiskState) {
  for (int i = 0; i < this->editorWindow->getDocumentCount(); ++i) {
    refreshDocumentState(i, syncDiskState);
  }
}

void DocumentRefresh::syncDocumentFileState(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return;
  }
  const EditorWindow::DocumentFileState currentFileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!currentFileState.hasPath()) {
    EditorWindow::DocumentFileState resetState = currentFileState;
    resetState.fileState = FileState::Ok;
    resetState.readOnly = false;
    resetState.externallyModified = false;
    resetState.lastModifiedTime = 0;
    this->editorWindow->applyDocumentFileStateSnapshot(index, resetState);
    applyDocumentReadOnlyState(index);
    this->editorWindow->recomputeDocumentStatus(index);
    return;
  }

  gint64 modifiedTime = 0;
  bool readOnly = false;
  FileState queriedFileState = FileState::Inaccessible;
  if (!DocumentState::query_file_state(currentFileState.path, modifiedTime,
                                       readOnly, queriedFileState)) {
    EditorWindow::DocumentFileState inaccessibleState = currentFileState;
    inaccessibleState.fileState = queriedFileState;
    inaccessibleState.readOnly = false;
    inaccessibleState.externallyModified = false;
    inaccessibleState.lastModifiedTime = 0;
    this->editorWindow->applyDocumentFileStateSnapshot(index, inaccessibleState);
    applyDocumentReadOnlyState(index);
    this->editorWindow->recomputeDocumentStatus(index);
    return;
  }

  const bool externallyModified =
      currentFileState.lastModifiedTime != 0 &&
      modifiedTime != currentFileState.lastModifiedTime;
  EditorWindow::DocumentFileState updatedState = currentFileState;
  updatedState.fileState = queriedFileState;
  updatedState.readOnly = readOnly;
  updatedState.externallyModified = externallyModified;
  updatedState.lastModifiedTime = modifiedTime;
  this->editorWindow->applyDocumentFileStateSnapshot(index, updatedState);
  applyDocumentReadOnlyState(index);
  this->editorWindow->recomputeDocumentStatus(index);
}

void DocumentRefresh::acknowledgeFileState(int index) {
  if (!this->editorWindow->hasDocument(index)) {
    return;
  }

  EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  fileState.externallyModified = false;
  this->editorWindow->applyDocumentFileStateSnapshot(index, fileState);
  refreshDocumentState(index, true);
}

void DocumentRefresh::checkOpenDocumentFileStates() {
  int originalIndex = this->editorWindow->getCurrentDocumentIndex();

  for (int i = 0; i < this->editorWindow->getDocumentCount(); ++i) {
    if (i >= this->editorWindow->getDocumentCount()) {
      break;
    }

    if (this->editorWindow->checkExternalModificationForDocument(i)) {
      break;
    }
  }

  if (originalIndex >= 0 &&
      originalIndex < this->editorWindow->getDocumentCount()) {
    this->editorWindow->switchToDocument(originalIndex);
  }
}

void DocumentRefresh::refreshDocumentStatesFromWorkspace(bool allowPrompts) {
  if (allowPrompts) {
    checkOpenDocumentFileStates();
    return;
  }

  refreshPeriodicDocumentStates();
}

void DocumentRefresh::refreshPeriodicDocumentStates() {
  const int currentIndex = this->editorWindow->getCurrentDocumentIndex();
  if (this->editorWindow->hasDocument(currentIndex)) {
    syncDocumentFileState(currentIndex);
  }
}

void DocumentRefresh::applyDocumentEolMode(int index) {
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  const EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!editorState.scintilla) {
    return;
  }

  int eolMode = DocumentIO::scintilla_eol_mode(fileState.eolMode);
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETEOLMODE,
                         eolMode, 0);
  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_CONVERTEOLS,
                         eolMode, 0);
}

void DocumentRefresh::applyDocumentReadOnlyState(int index) {
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  const EditorWindow::DocumentFileState fileState =
      this->editorWindow->getDocumentFileStateSnapshot(index);
  if (!editorState.scintilla) {
    return;
  }

  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETREADONLY,
                         fileState.readOnly ? 1 : 0, 0);
}

void DocumentRefresh::setDocumentModified(int index, bool modified) {
  if (!this->editorWindow->hasDocument(index)) {
    return;
  }

  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (editorState.modified != modified) {
    this->editorWindow->setDocumentModifiedFlag(index, modified);
    refreshDocumentState(index, false);
  }
}

void DocumentRefresh::set_document_modified(bool modified) {
  setDocumentModified(this->editorWindow->getCurrentDocumentIndex(), modified);
}

void DocumentRefresh::markDocumentSaved(int index) {
  const EditorWindow::DocumentEditorState editorState =
      this->editorWindow->getDocumentEditorState(index);
  if (!editorState.scintilla) {
    return;
  }

  scintilla_send_message(SCINTILLA(editorState.scintilla), SCI_SETSAVEPOINT, 0,
                         0);
  setDocumentModified(index, false);
}
