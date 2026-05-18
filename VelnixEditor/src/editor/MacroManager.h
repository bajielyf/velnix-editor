#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>

class EditorWindow;

class MacroManager {
public:
  explicit MacroManager(EditorWindow* editorWindow);
  ~MacroManager() = default;

  // Macro recording
  void startRecording();
  void stopRecording();
  bool isRecording() const { return _isRecording; }

  // Macro playback
  void playbackMacro(const Macro& macro);

  // Macro management
  void saveCurrentMacro(const std::string& name);
  void promptAndSaveCurrentMacro();
  void showPlaybackDialog();
  void showMacroManagementDialog();
  void deleteMacro(int id);
  bool renameMacro(int id, const std::string& newName);
  void loadMacros();
  void saveMacros();
  void reloadMacros();
  const std::vector<MacroShortcut>& getMacros() const { return _macros; }

  // Event handling
  void handleScintillaNotification(SCNotification* notification);
  void recordSearchAction(MacroAppCommand command,
                          const std::string& findPattern,
                          const std::string& replaceText,
                          bool caseSensitive,
                          bool wholeWord,
                          bool regex);

  // Menu integration
  void addMacroMenuItems(GtkWidget* macroMenu);

private:
  EditorWindow* _editorWindow;
  GtkWidget* _macroMenu = nullptr;
  bool _isRecording;
  bool _isPlayingBack = false;
  Macro _currentMacro;

  std::vector<MacroShortcut> _macros;
  int _nextMacroId;

  // Helper methods
  void addMacroStep(int message, uptr_t wParam, sptr_t lParam, const char* sParam = nullptr, int type = 0);
  void executeMacroStep(const RecordedMacroStep& step);
  std::string getMacrosFilePath() const;
  std::string generateDefaultMacroName() const;
  void refreshMacroMenu();
  MacroShortcut *findMacroById(int id);
  const MacroShortcut *findMacroById(int id) const;
  void showInfoDialog(const std::string& title, const std::string& message) const;
  void showWarningDialog(const std::string& title, const std::string& message) const;
};
