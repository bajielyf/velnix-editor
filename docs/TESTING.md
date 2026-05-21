# Velnix Editor Testing

This document records the currently available test and validation entry points.
It is intentionally practical: these are the commands a release preparer should
be able to run or review before publishing the first standalone release.

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
- `test_document_state`
- `test_config_store`
- `test_document_operations`

Build them explicitly when preparing a release:

```bash
cmake --build build --target test_gtk -j4
cmake --build build --target test_document_state -j4
cmake --build build --target test_config_store -j4
cmake --build build --target test_document_operations -j4
```

## 3. Run Non-GUI Tests

After building, run the non-GUI test binaries directly:

```bash
./build/VelnixEditor/test_document_state
./build/VelnixEditor/test_config_store
./build/VelnixEditor/test_document_operations
```

These binaries are not currently registered with CTest. If CTest integration is
added later, update this document and the release checklist at the same time.

## 4. Manual Smoke Test

Before a release, run the main editor and verify the following flow:

- launch `./build/VelnixEditor/velnix-editor`
- create a new document
- open an existing text file
- edit and save the document
- use find and replace
- verify selected-keyword highlighting: normal selection follows the smart
  highlight preference, while double-click word selection highlights matching
  occurrences
- switch syntax highlighting by language or file extension
- convert encoding through the `Encoding` menu
- close and reopen the application to confirm configuration persistence

## 5. Packaging Smoke Test

After running `./scripts/build-deb.sh`, install the generated package in a clean
or disposable environment and verify:

- `/usr/bin/velnix-editor` starts successfully
- the desktop entry appears as `Velnix Editor`
- the installed README is present under the package documentation directory
- a basic open, edit, save flow works from the installed binary
