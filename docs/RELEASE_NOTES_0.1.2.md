# Velnix Editor 0.1.2 Release Notes

Velnix Editor 0.1.2 is a patch release focused on file-opening workflows,
macro playback reliability, shortcut startup behavior, and Debian package
release artifacts.

## Highlights

- Optional command-line file arguments are supported:
  `velnix-editor [file ...]`.
- Files can be opened by drag-and-drop in the main window, editor area, and tab
  labels.
- Macro playback now preserves recorded search result actions more reliably,
  including batched find-all playback.
- Shortcut handling is fixed for the first launch after installing a package.
- Debian package output now includes `SHA256SUMS` for release verification.

## Downloads

Download the Debian package and checksum file from this release page:

- `velnix-editor_0.1.2_amd64.deb`
- `SHA256SUMS`

Verify the package from the release directory with:

```bash
sha256sum -c SHA256SUMS
```

Install it with:

```bash
sudo dpkg -i velnix-editor_0.1.2_amd64.deb
```

If your system reports missing dependencies, repair the installation with:

```bash
sudo apt install -f
```

The package installs:

- the `velnix-editor` executable
- a desktop launcher entry
- bundled project documentation

## Build From Source

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/VelnixEditor/velnix-editor [file ...]
```

To build a local Debian package:

```bash
./scripts/build-deb.sh
```

The generated package and `SHA256SUMS` file are written to `dist/`.

## Known Limitations

- Very large files or broad search scopes still need additional performance
  validation and tuning.
- Theme customization and plugin support are planned for later releases.
- Drag-and-drop and command-line file opening are available, but additional
  validation is still planned for duplicate-open behavior and failed path
  feedback.
- End-user documentation is still intentionally compact and will expand as the
  project moves beyond the early release line.

## Changes Since 0.1.1

- Added optional command-line file arguments after restored session tabs.
- Added drag-and-drop file opening for the main window, editor area, and tab
  labels.
- Added `SHA256SUMS` generation for Debian package output.
- Fixed first-launch shortcut activation after package installation.
- Fixed macro execution omissions for recorded search workflows.
- Fixed Debian package author/maintainer metadata.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
