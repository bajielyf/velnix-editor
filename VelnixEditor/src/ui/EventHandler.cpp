#include "ui/EventHandler.h"
#include "ui/EditorWindow.h"
#include "editor/SearchManager.h"

#include <cstdio>

EventHandler::EventHandler(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

void EventHandler::on_tab_close_clicked(GtkWidget *button, gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  int index = editorWindow->findDocumentIndexByCloseButton(button);
  editorWindow->closeDocument(index);
}

void EventHandler::on_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page,
                                        guint page_num, gpointer data) {
  (void)notebook;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  int index = editorWindow->findDocumentIndexByScintilla(page);
  editorWindow->activateDocument((index >= 0) ? index : static_cast<int>(page_num),
                                 false);
  editorWindow->applyCurrentSyntaxHighlighting();
  editorWindow->refreshBraceHighlighting(page);
  editorWindow->refreshDocumentState(editorWindow->getCurrentDocumentIndex(), true);
}

void EventHandler::on_notebook_page_reordered(GtkNotebook *notebook,
                                           GtkWidget *page, guint page_num,
                                           gpointer data) {
  (void)notebook;
  (void)page;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->reorderDocumentsToMatchNotebook();
  editorWindow->activateDocument(static_cast<int>(page_num), false);
}

void EventHandler::on_scintilla_notify(ScintillaObject *sci, gint id,
                                    SCNotification *notification,
                                    gpointer data) {
  (void)id;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!notification) {
    return;
  }

  int index = editorWindow->findDocumentIndexByScintilla(GTK_WIDGET(sci));
  if (index < 0) {
    return;
  }

  if (notification->nmhdr.code == SCN_SAVEPOINTLEFT) {
    editorWindow->setDocumentModified(index, true);
  } else if (notification->nmhdr.code == SCN_SAVEPOINTREACHED) {
    editorWindow->setDocumentModified(index, false);
  } else if (notification->nmhdr.code == SCN_MODIFIED ||
             notification->nmhdr.code == SCN_ZOOM) {
    if (notification->nmhdr.code == SCN_ZOOM || notification->linesAdded != 0) {
      editorWindow->refreshLineNumberStyle(GTK_WIDGET(sci),
                                           notification->nmhdr.code == SCN_ZOOM);
    }
    // Handle auto-refresh for search results on text modifications
    if (notification->nmhdr.code == SCN_MODIFIED &&
        (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) {
      editorWindow->refreshSearchResultsAfterModification();
    }
  } else if (notification->nmhdr.code == SCN_UPDATEUI) {
    editorWindow->refreshDynamicHighlights(index, notification->updated);
  } else if (notification->nmhdr.code == SCN_DOUBLECLICK) {
    editorWindow->handleSelectedKeywordDoubleClick(index);
  }

  // Handle macro recording
  editorWindow->handleMacroScintillaNotification(notification);
}

gboolean EventHandler::on_scintilla_button_press_event(GtkWidget *widget,
                                                       GdkEventButton *event,
                                                       gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!editorWindow->isColumnEditorEnabled() || event->button != 1) {
    return FALSE;
  }

  editorWindow->beginColumnEditorDrag(widget, static_cast<int>(event->x),
                                      static_cast<int>(event->y));
  return TRUE;
}

gboolean EventHandler::on_scintilla_button_release_event(GtkWidget *widget,
                                                         GdkEventButton *event,
                                                         gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!editorWindow->isColumnEditorDragging() || event->button != 1) {
    return FALSE;
  }

  editorWindow->endColumnEditorDrag(widget, static_cast<int>(event->x),
                                    static_cast<int>(event->y));
  return TRUE;
}

gboolean EventHandler::on_scintilla_motion_notify_event(GtkWidget *widget,
                                                        GdkEventMotion *event,
                                                        gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!editorWindow->isColumnEditorDragging()) {
    return FALSE;
  }

  editorWindow->updateColumnEditorDrag(widget, static_cast<int>(event->x),
                                       static_cast<int>(event->y));
  return TRUE;
}

gboolean EventHandler::on_scintilla_scroll_event(GtkWidget *widget,
                                               GdkEventScroll *event,
                                               gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!editorWindow->isCtrlMouseWheelZoomEnabled()) {
    return FALSE;
  }

  if (!(event->state & GDK_CONTROL_MASK)) {
    return FALSE;
  }

  int zoomLevel = static_cast<int>(scintilla_send_message(SCINTILLA(widget), SCI_GETZOOM, 0, 0));
  if (event->direction == GDK_SCROLL_UP ||
      (event->direction == GDK_SCROLL_SMOOTH && event->delta_y < 0)) {
    zoomLevel += 1;
  } else if (event->direction == GDK_SCROLL_DOWN ||
             (event->direction == GDK_SCROLL_SMOOTH && event->delta_y > 0)) {
    zoomLevel -= 1;
  } else {
    return FALSE;
  }

  scintilla_send_message(SCINTILLA(widget), SCI_SETZOOM, zoomLevel, 0);
  return TRUE;
}

gboolean EventHandler::on_window_delete_event(GtkWidget *widget, GdkEvent *event,
                                           gpointer data) {
  (void)widget;
  (void)event;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->saveSession();
  gtk_main_quit();
  return FALSE; // Allow window to close
}

gboolean EventHandler::on_window_focus_in_event(GtkWidget *widget, GdkEvent *event,
                                             gpointer data) {
  (void)widget;
  (void)event;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->refreshDocumentStatesFromWorkspace(true);
  return FALSE;
}

gboolean EventHandler::on_window_key_press_event(GtkWidget *widget,
                                                 GdkEventKey *event,
                                                 gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (event && event->keyval == GDK_KEY_F11 &&
      (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK |
                       GDK_SUPER_MASK)) == 0) {
    editorWindow->toggleFullscreenMode();
    return TRUE;
  }
  return FALSE;
}

gboolean EventHandler::on_window_state_event(GtkWidget *widget,
                                             GdkEventWindowState *event,
                                             gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (event && (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)) {
    const bool isFullscreen =
        (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
    if (editorWindow->isFullscreenMode() != isFullscreen) {
      editorWindow->setFullscreenMode(isFullscreen);
    }
  }
  return FALSE;
}

void EventHandler::enable_file_drop_target(GtkWidget *widget,
                                           EditorWindow *editorWindow) {
  if (!widget || !editorWindow) {
    return;
  }

  static gchar textUriListTarget[] = "text/uri-list";
  static GtkTargetEntry targets[] = {{textUriListTarget, 0, 0}};

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, targets,
                    G_N_ELEMENTS(targets), GDK_ACTION_COPY);
  g_signal_connect(widget, "drag-data-received",
                   G_CALLBACK(EventHandler::on_file_drop_received),
                   editorWindow);
}

void EventHandler::on_file_drop_received(GtkWidget *widget,
                                         GdkDragContext *context, gint x,
                                         gint y,
                                         GtkSelectionData *selectionData,
                                         guint info, guint time,
                                         gpointer data) {
  (void)widget;
  (void)x;
  (void)y;
  (void)info;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  if (!editorWindow || !selectionData) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  gchar **uris = gtk_selection_data_get_uris(selectionData);
  if (!uris) {
    gtk_drag_finish(context, FALSE, FALSE, time);
    return;
  }

  bool openedAny = false;
  for (gchar **uri = uris; *uri; ++uri) {
    GError *error = nullptr;
    gchar *filename = g_filename_from_uri(*uri, nullptr, &error);
    if (filename) {
      openedAny = editorWindow->openDocumentPath(filename) || openedAny;
      g_free(filename);
    }
    if (error) {
      g_error_free(error);
    }
  }

  g_strfreev(uris);
  gtk_drag_finish(context, openedAny ? TRUE : FALSE, FALSE, time);
}

gboolean EventHandler::on_periodic_file_state_refresh(gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->refreshDocumentStatesFromWorkspace(false);
  return G_SOURCE_CONTINUE;
}

void EventHandler::on_new_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->createNewDocument();
}

void EventHandler::on_open_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->open_file_dialog();
}

void EventHandler::on_reload_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->reload_current_document();
}

void EventHandler::on_save_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->save_file();
}

void EventHandler::on_save_as_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->saveDocumentAs(editorWindow->getCurrentDocumentIndex());
}

void EventHandler::on_save_all_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->save_all_documents();
}

void EventHandler::on_close_all_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->close_all_documents();
}

void EventHandler::on_exit_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->saveSession();
  gtk_main_quit();
}

void EventHandler::on_undo_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  GtkWidget *sci = editorWindow->getCurrentScintilla();
  if (sci) {
    scintilla_send_message(SCINTILLA(sci), SCI_UNDO, 0, 0);
  }
}

void EventHandler::on_redo_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  GtkWidget *sci = editorWindow->getCurrentScintilla();
  if (sci) {
    scintilla_send_message(SCINTILLA(sci), SCI_REDO, 0, 0);
  }
}

void EventHandler::on_cut_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  GtkWidget *sci = editorWindow->getCurrentScintilla();
  if (sci) {
    scintilla_send_message(SCINTILLA(sci), SCI_CUT, 0, 0);
  }
}

void EventHandler::on_copy_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  GtkWidget *sci = editorWindow->getCurrentScintilla();
  if (sci) {
    scintilla_send_message(SCINTILLA(sci), SCI_COPY, 0, 0);
  }
}

void EventHandler::on_paste_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  GtkWidget *sci = editorWindow->getCurrentScintilla();
  if (sci) {
    scintilla_send_message(SCINTILLA(sci), SCI_PASTE, 0, 0);
  }
}

void EventHandler::on_column_editor_toggled(GtkWidget *widget, gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->setColumnEditorEnabled(
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

void EventHandler::on_uppercase_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->convertSelectionToUppercase();
}

void EventHandler::on_lowercase_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->convertSelectionToLowercase();
}

void EventHandler::on_title_case_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->convertSelectionToTitleCase();
}

void EventHandler::on_trim_trailing_whitespace_activate(GtkWidget *widget,
                                                        gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->trimTrailingWhitespace();
}

void EventHandler::on_spaces_to_tabs_activate(GtkWidget *widget,
                                              gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->convertSpacesToTabs();
}

void EventHandler::on_tabs_to_spaces_activate(GtkWidget *widget,
                                              gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->convertTabsToSpaces();
}

void EventHandler::on_convert_encoding_activate(GtkWidget *widget,
                                                gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  const auto encodingValue = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(widget), "target-encoding"));
  const bool useBom = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(widget), "target-use-bom")) != 0;
  editorWindow->convertCurrentDocumentEncoding(
      static_cast<FileEncoding>(encodingValue), useBom);
}

void EventHandler::on_find_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->showFindDialog();
}

void EventHandler::on_replace_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->showReplaceDialog();
}

void EventHandler::on_go_to_line_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->showGoToLineDialog();
}

void EventHandler::on_search_results_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->toggleSearchResultsPanel();
}

void EventHandler::on_fullscreen_toggled(GtkWidget *widget, gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->setFullscreenMode(
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

void EventHandler::on_word_wrap_toggled(GtkWidget *widget, gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->setWordWrapEnabled(
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)), true);
}

void EventHandler::on_show_whitespace_toggled(GtkWidget *widget,
                                              gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->setWhitespaceMarkersVisible(
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

void EventHandler::on_show_eol_markers_toggled(GtkWidget *widget,
                                               gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->setEolMarkersVisible(
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
}

void EventHandler::on_zoom_in_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->zoomIn();
}

void EventHandler::on_zoom_out_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->zoomOut();
}

void EventHandler::on_reset_zoom_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->resetZoom();
}
