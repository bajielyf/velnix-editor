# Velnix Editor Known Limitations

This document records known limitations for the Velnix Editor 0.1.2 release.

## 1. Search Responsiveness

Search and replace workflows are available, including result navigation,
replace-all support, result refresh, background find-all processing, and
cancellation. Very large files or broad multi-document searches still need more
performance validation and tuning.

Planned follow-up:

- tune large-file and many-document search performance
- improve progress/status detail for expensive result sets
- add clearer feedback when a search produces a large result set or takes a
  long time

## 2. Theme and Appearance Customization

The current release includes the core native editing workflow, but full theme
customization is not included yet.

Planned follow-up:

- add user-selectable editor color themes
- improve UI appearance settings
- persist theme-related preferences cleanly

## 3. File Opening Feedback

Files can be opened through the menu, file dialogs, command-line arguments, and
single-file or multi-file drag-and-drop targets in the main window, editor area,
and tab labels. Duplicate paths are normalized and switch to the existing tab,
but some edge-case feedback still needs refinement.

Planned follow-up:

- improve feedback for unsupported or partially handled drop payloads
- expand validation for paths provided by desktop launchers and terminals

## 4. Plugin Support

Velnix Editor 0.1.2 does not include a plugin system. This release keeps
the extension boundary intentionally simple while the core editor behavior
stabilizes.

Planned follow-up:

- define plugin loading and capability boundaries
- decide how plugins should be configured and packaged
- document compatibility expectations before exposing a public plugin API

## 5. End-User Documentation

The repository includes build, installation, testing, encoding, roadmap,
third-party notice, changelog, and release documentation. A fuller end-user
manual is still future work.

Planned follow-up:

- add task-focused help for editing, search, macros, shortcuts, and encoding
- add troubleshooting notes for common packaging and runtime issues
- keep release notes and user documentation aligned as features grow
