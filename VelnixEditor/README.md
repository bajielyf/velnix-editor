# Velnix Editor Module Guide

`VelnixEditor/` contains the Linux GTK application layer.

## Main Entry Points

- `src/app/main.cpp`: application bootstrap
- `src/ui/EditorWindow.*`: top-level window coordination
- `src/editor/`: document, search, macro, and highlighting logic
- `src/config/`: preferences, shortcuts, recent files, config paths
- `src/core/EditorTypes.*`: shared editor data types

## Build Target

The module builds the `velnix-editor` executable.

## Default Config Path

Configuration files are stored in `~/.config/velnix-editor/` unless a custom
directory is configured.
