# Velnix Editor Known Limitations

This document records known limitations for the Velnix Editor 0.1.0 release
line and the main follow-up areas planned after the first public release.

## 1. Search Responsiveness

Search and replace workflows are available, including result navigation and
replace-all support. Find-all result building runs in the background, but very
large files or broad search scopes still need release validation and tuning.

Planned follow-up:

- tune large-file and many-document search performance
- improve progress and cancellation visibility for expensive result sets
- add clearer feedback for expensive search operations

## 2. Theme and Appearance Customization

The first release focuses on the core native editing workflow. Full theme
customization is not included yet.

Planned follow-up:

- add user-selectable editor color themes
- improve UI appearance settings
- persist theme-related preferences cleanly

## 3. Drag-and-Drop File Opening

Files can be opened through the menu and file dialogs, but drag-and-drop file
opening for the main window and tab area is not part of the 0.1.0 release.

Planned follow-up:

- support dropping files onto the main editor window
- handle duplicate-open detection consistently for dropped files
- surface clear feedback when a dropped path cannot be opened

## 4. Selected-Word Occurrence Highlighting

Syntax highlighting and search result highlighting are available. Automatic
highlighting of all occurrences of the currently selected word or token is not
included yet.

Planned follow-up:

- detect suitable selected words or tokens
- highlight matching occurrences in the active document
- avoid noisy highlighting for large files or overly broad selections

## 5. Plugin Support

Velnix Editor 0.1.0 does not include a plugin system. The first release keeps
the extension boundary intentionally simple while the core editor behavior
stabilizes.

Planned follow-up:

- define plugin loading and capability boundaries
- decide how plugins should be configured and packaged
- document compatibility expectations before exposing a public plugin API

## 6. End-User Documentation

The repository includes build, installation, testing, encoding, roadmap,
third-party notice, changelog, and release documentation. A fuller end-user
manual is still future work.

Planned follow-up:

- add task-focused help for editing, search, macros, shortcuts, and encoding
- add troubleshooting notes for common packaging and runtime issues
- keep release notes and user documentation aligned as features grow

## 7. First-Release Validation Scope

The 0.1.0 release is intended as the first public baseline. Before publishing a
final artifact, validate the generated Debian package in a clean or disposable
Linux environment.

Recommended validation:

- launch from both terminal and desktop entry
- open, edit, save, reload, and close files
- test search, replace, syntax highlighting, and encoding conversion
- verify preferences, shortcuts, macros, recent files, and session restore
- confirm installed documentation under `/usr/share/doc/velnix-editor/`
