# Velnix Editor Known Limitations

This document records known limitations for the Velnix Editor 0.1.7 release.

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

## 4. Custom Keyword Highlight Refresh

Custom keyword highlights are scoped to the current document and can be
assigned or cleared by color. The current refresh cache primarily tracks
document length, so a same-length content replacement may not always trigger an
immediate custom-highlight refresh in every editing path.

Planned follow-up:

- track content revisions instead of relying primarily on document length
- add focused tests for same-length replacements and highlight refresh
- validate custom highlighting performance on large documents

## 5. Legacy Encoding Detection

Legacy encoding detection uses ICU statistical results together with
script-aware validation. Some short files and similar single-byte encodings do
not contain enough evidence for an unambiguous result. Use **Reopen with
Encoding** when a file is decoded incorrectly.

Release validation on the current Linux/ICU 74.2 environment found that the
Debug `test_document_operations` target fails `test_decode_gb18030`: the short
GB18030 sample is not identified as `Gb18030`. The detection implementation and
test are unchanged since `v0.1.5`. Investigate this failure before publishing;
Release builds can skip assert-based checks and are not sufficient validation.

Planned follow-up:

- expand regression samples for short and mixed-script documents
- improve confidence reporting and manual-selection feedback
- continue validating detection performance on very large files

## 6. Update Checks

Update checks retrieve a plain-text version from the project website and
therefore require working network access. Automatic check failures remain
silent to avoid interrupting startup; manual checks report the failure.

Planned follow-up:

- provide a user-facing preference for automatic update checks
- improve diagnostics for proxy, TLS, and connectivity failures
- consider publishing signed update metadata if the workflow expands beyond
  version discovery

## 7. Plugin Support

Velnix Editor 0.1.7 does not include a plugin system. This release keeps
the extension boundary intentionally simple while the core editor behavior
stabilizes.

Planned follow-up:

- define plugin loading and capability boundaries
- decide how plugins should be configured and packaged
- document compatibility expectations before exposing a public plugin API

## 8. End-User Documentation

The repository includes build, installation, testing, encoding, roadmap,
third-party notice, changelog, and release documentation. A fuller end-user
manual is still future work.

Planned follow-up:

- add task-focused help for editing, search, macros, shortcuts, and encoding
- add troubleshooting notes for common packaging and runtime issues
- keep release notes and user documentation aligned as features grow
