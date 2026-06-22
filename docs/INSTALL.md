# Velnix Editor Installation and Packaging

This document is for developers and testers who need to build, run, or package
Velnix Editor.

## 1. Build From Source

Requirements:

- CMake 3.16 or newer
- a C++17-capable compiler
- GTK 3 development files

Build commands:

```bash
cmake -S . -B build
cmake --build build -j4
```

After the build completes, the main executable is available at:

```bash
./build/VelnixEditor/velnix-editor
```

## 2. Run Locally

Run the application directly:

```bash
./build/VelnixEditor/velnix-editor [file ...]
```

File arguments are optional. When one or more paths are provided, Velnix Editor
opens them after any tabs restored from the previous session.

Default user configuration directory:

```bash
~/.config/velnix-editor/
```

## 3. Build a Debian Package

The repository provides a single Debian packaging entry point:

```bash
./scripts/build-deb.sh
```

Default behavior:

- reads the project version from the top-level `CMakeLists.txt`
- builds in `Release` mode
- stages the install tree under `build/deb-root/`
- writes the final `.deb` package to `dist/`
- writes `dist/SHA256SUMS` for the generated package

Version note: the packaging script reads the top-level CMake project version.
The top-level `CMakeLists.txt` is the release version source. The application
subproject uses that value during normal repository builds and only declares a
fallback version when built standalone. Each release should still confirm that
the package version, changelog, release tag, website metadata, and release notes
all use the same value.

## 4. Common Packaging Overrides

The script supports environment variable overrides. Common options include:

- `VERSION`
- `ARCH`
- `BUILD_TYPE`
- `PACKAGE_NAME`
- `MAINTAINER`
- `HOMEPAGE`
- `DEPENDS` (defaults to dependencies detected with `dpkg-shlibdeps`)
- `BUILD_DIR`
- `PKG_ROOT`
- `OUT_DIR`
- `JOBS`

Example:

```bash
VERSION=0.1.3 BUILD_TYPE=Release ./scripts/build-deb.sh
```

## 5. Install the Generated Debian Package

After packaging, install the package with `dpkg`:

```bash
sudo dpkg -i dist/velnix-editor_<version>_<arch>.deb
```

The installed package includes:

- `/usr/bin/velnix-editor`
- the desktop entry
- installed README, changelog, and license/copyright documentation

## 6. Suggested Pre-Release Checks

Before preparing a release, verify at minimum:

- source build completes successfully
- `.deb` package generation completes successfully
- package installation works in the target environment
- the application can start, open, edit, and save text files
- the desktop entry is visible and named `Velnix Editor`
- build, run, version, and download instructions are consistent across docs

See [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for the full pre-release checklist.
