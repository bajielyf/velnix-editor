# Velnix Editor 0.1.3 Release Notes

Velnix Editor 0.1.3 is a patch release focused on smoother search and replace
workflows, per-document custom keyword highlighting, and more consistent dialog
placement.

## Highlights

- Find and replace dialogs are now non-modal, allowing editing to continue
  while either dialog remains open.
- Find and replace include a backwards-search option for navigating matches in
  reverse order.
- Selected words or tokens can be assigned one of six custom highlight colors
  for the current document.
- Custom highlights are available from the View menu and editor context menu,
  and can be cleared by color or all at once.
- Find, replace, preferences, and keyboard shortcut dialogs open centered on
  the main editor window.

## Downloads

Download the Debian package and checksum file from this release page:

- `velnix-editor_0.1.3_amd64.deb`
- `SHA256SUMS`

Verify the package from the release directory with:

```bash
sha256sum -c SHA256SUMS
```

Install it with:

```bash
sudo dpkg -i velnix-editor_0.1.3_amd64.deb
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
- Custom keyword highlights are document-local. The current refresh cache may
  miss an immediate refresh after some same-length content replacements.
- Theme customization and plugin support are planned for later releases.
- End-user documentation remains intentionally compact.

## Changes Since 0.1.2

- Changed find and replace dialogs from modal to non-modal windows.
- Added backwards-search controls to find and replace.
- Centered preferences and keyboard shortcut dialogs on the main editor
  window.
- Added per-document custom keyword highlighting with six colors.
- Added View-menu and editor context-menu access for applying and clearing
  custom highlights.
- Fixed the purple custom-highlight color value.
- Documented the remaining same-length refresh-cache limitation for custom
  highlights.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
