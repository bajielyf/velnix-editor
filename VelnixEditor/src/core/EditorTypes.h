#pragma once

#include <gtk/gtk.h>

#define GTK 1
#include <Scintilla.h>
#include <ScintillaWidget.h>

#include <string>
#include <vector>

enum class FileEncoding { Ansi, Utf8, Utf16LE, Utf16BE };

enum class EolMode { CrLf, Cr, Lf };

enum class FileState { Ok, Missing, Inaccessible };

enum class DocumentStatus {
  Untitled,
  Saved,
  Modified,
  ExternallyModified,
  Missing,
  Inaccessible
};

struct Document {
  GtkWidget *scintilla;
  GtkWidget *tabLabel;
  GtkWidget *tabTextLabel;
  GtkWidget *closeButton;
  int bufferId;
  std::string filePath;
  std::string title;
  bool modified;
  FileEncoding encoding;
  bool useBom;
  EolMode eolMode;
  std::string lexerName;
  bool readOnly;
  bool externallyModified;
  gint64 lastModifiedTime;
  FileState fileState;
  DocumentStatus status;
  int cachedLineNumberDigits = 0;
  sptr_t cachedBraceCursorPosition = INVALID_POSITION;
  sptr_t cachedBraceHighlightStart = INVALID_POSITION;
  sptr_t cachedBraceHighlightEnd = INVALID_POSITION;
  sptr_t cachedBraceBadLightPosition = INVALID_POSITION;
  int cachedCurrentColumnHighlight = -1;
  bool cachedCurrentColumnHighlightEnabled = false;
  std::string cachedSelectedKeywordHighlight;
  sptr_t cachedSelectedKeywordDocumentLength = INVALID_POSITION;
  bool selectedKeywordHighlightFromDoubleClick = false;
  std::string selectedKeywordDoubleClickText;
};

struct ShortcutBinding {
  ShortcutBinding() = default;
  ShortcutBinding(const std::string &actionName, GtkWidget *item, guint key,
                  GdkModifierType mods, const std::string &displayText)
      : actionName(actionName),
        item(item),
        key(key),
        mods(mods),
        displayText(displayText),
        keys({key}),
        modifiers({mods}),
        displayTexts({displayText}),
        defaultDisplayTexts({displayText}) {}

  std::string actionName;
  GtkWidget *item;
  guint key;
  GdkModifierType mods;
  std::string displayText;
  std::vector<guint> keys;
  std::vector<GdkModifierType> modifiers;
  std::vector<std::string> displayTexts;
  std::vector<std::string> defaultDisplayTexts;
};

enum class MacroTypeIndex {
  mtUseLParameter,
  mtUseSParameter,
  mtMenuCommand,
  mtSavedSnR,
  mtAppCommand
};

enum class MacroAppCommand {
  FindNext = 1,
  FindPrevious,
  FindAll,
  ReplaceNext,
  ReplaceAll
};

struct RecordedMacroStep {
  int message = 0;
  uptr_t wParameter = 0;
  sptr_t lParameter = 0;
  std::string sParameter;
  MacroTypeIndex macroType = MacroTypeIndex::mtMenuCommand;

  RecordedMacroStep(int iMessage, uptr_t wParam, sptr_t lParam);
  explicit RecordedMacroStep(int iCommandID) : wParameter(iCommandID) {}

  RecordedMacroStep(int iMessage, uptr_t wParam, sptr_t lParam, const char* sParam, int type)
    : message(iMessage), wParameter(wParam), lParameter(lParam),
      sParameter((sParam != nullptr) ? sParam : ""),
      macroType(static_cast<MacroTypeIndex>(type)) {}

  bool isScintillaMacro() const { return macroType <= MacroTypeIndex::mtMenuCommand; }
  bool isMacroable() const;
};

using Macro = std::vector<RecordedMacroStep>;

struct MacroShortcut {
  std::string name;
  Macro macro;
  int id;
};
