#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build/package}"
PKG_ROOT="${PKG_ROOT:-${ROOT_DIR}/build/deb-root}"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/dist}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
PACKAGE_NAME="${PACKAGE_NAME:-velnix-editor}"
MAINTAINER="${MAINTAINER:-bajie <bajielyf@google.com>}"
HOMEPAGE="${HOMEPAGE:-https://bajielyf.github.io/velnix-editor}"
DEPENDS="${DEPENDS:-}"
VERSION="${VERSION:-}"
ARCH="${ARCH:-}"
JOBS="${JOBS:-}"

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: required tool not found: $1" >&2
        exit 1
    fi
}

cmake_project_version() {
    sed -nE 's/^project\(VelnixEditor VERSION ([^ )]+).*/\1/p' "${ROOT_DIR}/CMakeLists.txt" | head -n 1
}

sed_escape_replacement() {
    sed -e 's/[&|\\]/\\&/g'
}

detect_shlib_depends() {
    local output depends shlibdeps_dir

    shlibdeps_dir="${BUILD_DIR}/dpkg-shlibdeps"
    mkdir -p "${shlibdeps_dir}/debian"
    printf 'Source: %s\n\nPackage: %s\nArchitecture: %s\nDepends: ${shlibs:Depends}\nDescription: shlib dependency scan\n shlib dependency scan.\n' \
        "${PACKAGE_NAME}" "${PACKAGE_NAME}" "${ARCH}" > "${shlibdeps_dir}/debian/control"

    if ! output="$(
        cd "${shlibdeps_dir}"
        dpkg-shlibdeps -O -S"${PKG_ROOT}" -e"${PKG_ROOT}/usr/bin/velnix-editor"
    )"; then
        echo "error: dpkg-shlibdeps failed to generate shared library dependencies" >&2
        exit 1
    fi
    depends="$(printf '%s\n' "${output}" | sed -nE 's/^shlibs:Depends=//p')"

    if [[ -z "${depends}" ]]; then
        echo "error: dpkg-shlibdeps did not generate any shared library dependencies" >&2
        exit 1
    fi

    printf '%s\n' "${depends}"
}

sha256_file() {
    local path="$1"

    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "${path}"
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "${path}"
    else
        echo "error: required tool not found: sha256sum or shasum" >&2
        exit 1
    fi
}

if [[ -z "${VERSION}" ]]; then
    VERSION="$(cmake_project_version)"
fi

if [[ -z "${VERSION}" ]]; then
    echo "error: VERSION is empty and could not be read from CMakeLists.txt" >&2
    exit 1
fi

require_tool cmake
require_tool dpkg-deb
require_tool dpkg
require_tool dpkg-shlibdeps

if [[ -z "${ARCH}" ]]; then
    ARCH="$(dpkg --print-architecture)"
fi

if [[ -z "${JOBS}" ]]; then
    if command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    else
        JOBS="2"
    fi
fi

rm -rf "${PKG_ROOT}"
mkdir -p "${PKG_ROOT}/DEBIAN" "${OUT_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DVELNIX_EDITOR_VERSION="${VERSION}" \
    -DVELNIX_HOMEPAGE="${HOMEPAGE}" \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build "${BUILD_DIR}" --target velnix_editor --parallel "${JOBS}"
DESTDIR="${PKG_ROOT}" cmake --install "${BUILD_DIR}"

if [[ ! -x "${PKG_ROOT}/usr/bin/velnix-editor" ]]; then
    echo "error: expected binary was not installed: /usr/bin/velnix-editor" >&2
    exit 1
fi

if [[ -z "${DEPENDS}" ]]; then
    DEPENDS="$(detect_shlib_depends)"
fi

INSTALLED_SIZE="$(du -sk "${PKG_ROOT}" | awk '{print $1}')"
DEPENDS_ESCAPED="$(printf '%s' "${DEPENDS}" | sed_escape_replacement)"
PACKAGE_NAME_ESCAPED="$(printf '%s' "${PACKAGE_NAME}" | sed_escape_replacement)"
VERSION_ESCAPED="$(printf '%s' "${VERSION}" | sed_escape_replacement)"
ARCH_ESCAPED="$(printf '%s' "${ARCH}" | sed_escape_replacement)"
MAINTAINER_ESCAPED="$(printf '%s' "${MAINTAINER}" | sed_escape_replacement)"
INSTALLED_SIZE_ESCAPED="$(printf '%s' "${INSTALLED_SIZE}" | sed_escape_replacement)"
HOMEPAGE_ESCAPED="$(printf '%s' "${HOMEPAGE}" | sed_escape_replacement)"

sed \
    -e "s|@PACKAGE_NAME@|${PACKAGE_NAME_ESCAPED}|g" \
    -e "s|@VERSION@|${VERSION_ESCAPED}|g" \
    -e "s|@ARCH@|${ARCH_ESCAPED}|g" \
    -e "s|@DEPENDS@|${DEPENDS_ESCAPED}|g" \
    -e "s|@MAINTAINER@|${MAINTAINER_ESCAPED}|g" \
    -e "s|@INSTALLED_SIZE@|${INSTALLED_SIZE_ESCAPED}|g" \
    -e "s|@HOMEPAGE@|${HOMEPAGE_ESCAPED}|g" \
    "${ROOT_DIR}/packaging/deb/control.in" > "${PKG_ROOT}/DEBIAN/control"

chmod 0755 "${PKG_ROOT}/DEBIAN"
chmod 0755 "${PKG_ROOT}"
find "${PKG_ROOT}/usr" -type d -exec chmod 0755 {} +
find "${PKG_ROOT}/usr" -type f -exec chmod 0644 {} +
chmod 0755 "${PKG_ROOT}/usr/bin/velnix-editor"

DEB_PATH="${OUT_DIR}/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
SHA256SUMS_PATH="${OUT_DIR}/SHA256SUMS"
dpkg-deb --build --root-owner-group "${PKG_ROOT}" "${DEB_PATH}"

(
    cd "${OUT_DIR}"
    sha256_file "$(basename "${DEB_PATH}")" > "$(basename "${SHA256SUMS_PATH}")"
)

echo "${DEB_PATH}"
echo "${SHA256SUMS_PATH}"
