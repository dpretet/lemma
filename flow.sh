#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

usage() {
    printf 'Usage: %s {build|test|release|clean}\n' "$0" >&2
}

configure_debug() {
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
}

run_tests() {
    ctest --test-dir "$1" --output-on-failure
}

if [[ $# -ne 1 ]]; then
    usage
    exit 2
fi

case "$1" in
    clean)
        rm -rf "${BUILD_DIR}" "${ROOT_DIR}/dist"
        printf 'Removed generated build/ and dist/ files.\n'
        ;;
    build)
        configure_debug
        cmake --build "${BUILD_DIR}" --parallel
        ;;
    test)
        configure_debug
        cmake --build "${BUILD_DIR}" --parallel
        run_tests "${BUILD_DIR}"
        ;;
    release)
        RELEASE_BUILD_DIR="${BUILD_DIR}/release"
        STAGE_DIR="${BUILD_DIR}/release-stage"
        DIST_DIR="${ROOT_DIR}/dist"

        cmake -S "${ROOT_DIR}" -B "${RELEASE_BUILD_DIR}" \
            -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
        cmake --build "${RELEASE_BUILD_DIR}" --parallel
        run_tests "${RELEASE_BUILD_DIR}"

        rm -rf "${STAGE_DIR}"
        cmake --install "${RELEASE_BUILD_DIR}" --prefix "${STAGE_DIR}"
        mkdir -p "${STAGE_DIR}/licenses" "${DIST_DIR}"
        cat > "${STAGE_DIR}/README.txt" <<'EOF'
Lemma is currently a parser and build-toolchain smoke test, not yet a formal
verification engine.

Usage: ./bin/lemma <source.sv>

This package is built for one operating system and architecture. See the
licenses/ directory for bundled dependency notices.
EOF
        cp "${ROOT_DIR}/LICENSE" "${STAGE_DIR}/licenses/lemma-LICENSE"
        cp "${ROOT_DIR}/third_party/tree-sitter/LICENSE" \
            "${STAGE_DIR}/licenses/tree-sitter-LICENSE"
        cp "${ROOT_DIR}/third_party/tree-sitter/lib/src/unicode/LICENSE" \
            "${STAGE_DIR}/licenses/tree-sitter-unicode-LICENSE"
        cp "${ROOT_DIR}/third_party/tree-sitter-systemverilog/LICENSE" \
            "${STAGE_DIR}/licenses/tree-sitter-systemverilog-LICENSE"
        cp "${ROOT_DIR}/third_party/tsoding-arena/LICENSE" \
            "${STAGE_DIR}/licenses/tsoding-arena-LICENSE"

        case "$(uname -s)" in
            Darwin) PLATFORM="macos" ;;
            Linux) PLATFORM="linux" ;;
            *)
                printf 'Unsupported release platform: %s\n' "$(uname -s)" >&2
                exit 1
                ;;
        esac
        ARCH="$(uname -m)"
        ARCHIVE="${DIST_DIR}/lemma-${PLATFORM}-${ARCH}.tar.gz"
        tar -czf "${ARCHIVE}" -C "${STAGE_DIR}" .
        printf 'Release package created: %s\n' "${ARCHIVE}"
        ;;
    *)
        usage
        exit 2
        ;;
esac
