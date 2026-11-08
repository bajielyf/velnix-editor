# Velnix Editor Testing

This document records the currently available test and validation entry points.
It is intentionally practical: these are the commands a release preparer should
be able to run or review before publishing a release.

## 1. Build All Main Targets

```bash
cmake -S . -B build
cmake --build build -j4
```

This builds the main editor target and the targets included in the default
CMake build graph.

## 2. Build Test Targets Explicitly

The project currently defines these test-related targets in
`VelnixEditor/CMakeLists.txt`:

- `test_gtk`
- `test_scrollable_tabs` (automated GTK tab-strip interaction checks)
- `test_document_state`
- `test_config_store`
- `test_document_operations`
- `test_update_checker`

Build them explicitly when preparing a release:

```bash
cmake --build build --target test_gtk -j4
cmake --build build --target test_document_state -j4
cmake --build build --target test_config_store -j4
cmake --build build --target test_document_operations -j4
cmake --build build --target test_update_checker -j4
```

## 3. Run Non-GUI Tests

After building, run the non-GUI test binaries directly:

```bash
./build/VelnixEditor/test_document_state
./build/VelnixEditor/test_config_store
./build/VelnixEditor/test_document_operations
./build/VelnixEditor/test_update_checker
```

These binaries are not currently registered with CTest. If CTest integration is
added later, update this document and the release checklist at the same time.

Several tests use C++ `assert`, so run them in a Debug build to keep those
checks enabled. A successful Release test run alone does not validate them:

```bash
cmake -S . -B build/release-validation -DCMAKE_BUILD_TYPE=Debug
cmake --build build/release-validation --target test_document_state test_config_store test_document_operations test_update_checker -j4
./build/release-validation/VelnixEditor/test_document_state
./build/release-validation/VelnixEditor/test_config_store
./build/release-validation/VelnixEditor/test_document_operations
./build/release-validation/VelnixEditor/test_update_checker
```

## 4. Manual Smoke Test

Before a release, run the main editor and verify the following flow:

- launch `./build/VelnixEditor/velnix-editor`
- launch `./build/VelnixEditor/velnix-editor file1.txt file2.txt` and confirm
  restored session tabs remain first, followed by both files in separate tabs
- create a new document
- open an existing text file
- edit and save the document
- open the editor context menu and exercise the available editing actions
- use find and replace
- verify selected-keyword highlighting: normal selection follows the smart
  highlight preference, while double-click word selection highlights matching
  occurrences
- switch syntax highlighting by language or file extension
- convert encoding through the `Encoding` menu
- reopen a non-UTF file with an explicit encoding and verify lossless saving
- convert among LF, CRLF, and CR from both the Edit menu and status bar
- switch language, EOL mode, and encoding through the status-bar controls
- select ASCII and CJK text, including multiple selections, and verify the
  Unicode character count appears to the left of EOL and disappears on deselection
- change the UI language and verify the selection count text updates
- click the font selector text area and verify the dropdown opens; check the
  monospaced filter, font ordering, live font/size preview, and persisted choice
- verify Cancel leaves the editor font unchanged, and a configured missing
  font is preserved with a warning in preferences
- verify the Search Results button has a spaced downward chevron and hides
  the results panel when clicked
- restore a session containing a large unsaved document and verify its complete
  content, cursor, and scroll state survive restarting the editor
- run **Check for Updates** from the Help menu and confirm the result dialog
- close and reopen the application to confirm configuration persistence

## 5. Packaging Smoke Test

For the independently scrollable document tabs, run the GTK regression test
with a graphical display available:

```bash
cmake --build build --target test_scrollable_tabs -j4
./build/VelnixEditor/test_scrollable_tabs
```

It checks that arrows scroll without activating documents, tab clicks and
keyboard navigation still switch documents, and reordering, removal, and window
destruction keep the tab strip consistent. Synthesized GTK mouse press, motion,
and release events also check dragging tabs in both directions with file-drop
targets on their labels, and clicking active and inactive tab close buttons.
During dragging, the test checks the pass-through preview overlay, the faded
placeholder's unchanged width, and deferred reordering. Dropping or cancelling
must remove the preview and restore the tab's opacity; cancellation preserves
the original order. Set `VELNIX_TEST_DRAG_SCREENSHOT` to a PNG path to capture
the drag preview during the test.

After running `./scripts/build-deb.sh`, install the generated package in a clean
or disposable environment and verify:

- `/usr/bin/velnix-editor` starts successfully
- the desktop entry appears as `Velnix Editor`
- the installed README is present under the package documentation directory
- a basic open, edit, save flow works from the installed binary
