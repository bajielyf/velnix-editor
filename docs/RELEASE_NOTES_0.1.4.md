# Velnix Editor 0.1.4 Release Notes

Velnix Editor 0.1.4 is a patch release focused on update discovery, quicker
access to common editing actions, and refreshed application branding.

## Highlights

- Velnix Editor can now check the published website metadata for newer
  releases without blocking the editor interface.
- An automatic update check runs at most once per local calendar day after
  startup.
- The Help menu includes a manual **Check for Updates** action.
- The editor context menu now includes undo, redo, cut, copy, paste, column
  editing, case conversion, and whitespace operations.
- Context-menu actions are enabled only when they apply to the current editor
  state.
- The application logo has been refreshed.

## Downloads

Download the Debian package and checksum file from this release page:

- `velnix-editor_0.1.4_amd64.deb`
- `SHA256SUMS`

Verify the package from the release directory with:

```bash
sha256sum -c SHA256SUMS
```

Install it with:

```bash
sudo dpkg -i velnix-editor_0.1.4_amd64.deb
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
- Update checks require network access to the project website. Automatic check
  failures remain silent; manual checks report the error.
- Theme customization and plugin support are planned for later releases.
- End-user documentation remains intentionally compact.

## Changes Since 0.1.3

- Added asynchronous release-version retrieval and comparison.
- Added automatic daily and manual update-check workflows.
- Added localized update status, availability, and error dialogs.
- Added common Edit-menu commands to the editor context menu.
- Added context-sensitive enablement for the new context-menu commands.
- Refreshed the application logo asset.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
