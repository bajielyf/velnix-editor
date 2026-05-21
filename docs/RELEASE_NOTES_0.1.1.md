# Velnix Editor 0.1.1 Release Notes

Velnix Editor 0.1.1 is a patch release focused on selected-keyword occurrence
highlighting.

## Highlights

- Selected-keyword occurrence highlighting is available for the active
  document.
- Double-clicking a word highlights matching occurrences immediately.
- Normal text selection can highlight matching occurrences when smart keyword
  highlighting is enabled.

## Downloads

Download the Debian package from this release page:

- `velnix-editor_0.1.1_amd64.deb`

Install it with:

```bash
sudo dpkg -i velnix-editor_0.1.1_amd64.deb
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
./build/VelnixEditor/velnix-editor
```

To build a local Debian package:

```bash
./scripts/build-deb.sh
```

The generated package is written to `dist/`.

## Known Limitations

- Very large files or broad search scopes still need additional performance
  validation and tuning.
- Theme customization, drag-and-drop file opening, and plugin support are
  planned for later releases.
- End-user documentation is still intentionally compact and will expand as the
  project moves beyond the early release line.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
