#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_TESTS="${BUILD_TESTS:-ON}"
BUILD_PACKAGE="${BUILD_PACKAGE:-OFF}"
BUILD_DIR="${BUILD_DIR:-}"
SOURCE_DIR="${SOURCE_DIR:-.}"
JOBS="${JOBS:-$(nproc)}"
CLEAN="${CLEAN:-OFF}"
INSTALL_DEPS="${INSTALL_DEPS:-OFF}"
CODE_QUALITY="${CODE_QUALITY:-OFF}"
FORMAT_CHECK="${FORMAT_CHECK:-OFF}"
ENABLE_COVERAGE="${ENABLE_COVERAGE:-OFF}"
BUILD_SBOM="${BUILD_SBOM:-OFF}"
SBOM_ONLY="${SBOM_ONLY:-OFF}"
SBOM_OUTPUT_DIR="${SBOM_OUTPUT_DIR:-}"
CMAKE_EXTRA_ARGS="${CMAKE_EXTRA_ARGS:-}"
VERSION="${VERSION:-${BUILD_VERSION:-}}"
SKIP_BUILD_PROTOBUF="${SKIP_BUILD_PROTOBUF:-OFF}"
SKIP_BUILD_ABSL="${SKIP_BUILD_ABSL:-OFF}"
INSTALL_AFTER_BUILD="${INSTALL_AFTER_BUILD:-OFF}"

for arg in "$@"; do
  case "$arg" in
    debug)
      BUILD_TYPE="Debug"
      ;;
    release)
      BUILD_TYPE="Release"
      ;;
    --clean|clean)
      CLEAN="ON"
      ;;
    --install-deps)
      INSTALL_DEPS="ON"
      ;;
    --code-quality)
      CODE_QUALITY="ON"
      ;;
    --format-check)
      FORMAT_CHECK="ON"
      ;;
    --sbom)
      BUILD_SBOM="ON"
      ;;
    --sbom-only)
      BUILD_SBOM="ON"
      SBOM_ONLY="ON"
      ;;
    --skip-protobuf)
      SKIP_BUILD_PROTOBUF="ON"
      ;;
    --skip-absl)
      SKIP_BUILD_ABSL="ON"
      ;;
    test)
      BUILD_TESTS="ON"
      ;;
    package)
      BUILD_PACKAGE="ON"
      ;;
    install)
      INSTALL_AFTER_BUILD="ON"
      ;;
  esac
done

# Keep derived defaults coherent after parsing args.
if [[ -z "${BUILD_DIR}" ]]; then
  BUILD_DIR="build-${BUILD_TYPE,,}"
fi

log() {
  printf '[build.sh] %s\n' "$*"
}

install_deps() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  local deps_dir="${script_dir}/deps"

  local distro="unknown"
  if [[ -f /etc/os-release ]]; then
    local os_id="" os_codename="" os_version_id=""
    os_id="$(. /etc/os-release && echo "${ID:-}")"
    os_codename="$(. /etc/os-release && echo "${VERSION_CODENAME:-}")"
    os_version_id="$(. /etc/os-release && echo "${VERSION_ID:-}")"

    case "${os_id}:${os_codename}" in
      ubuntu:noble) distro="ubuntu24" ;;
      debian:trixie) distro="trixie" ;;
      *)
        case "${os_id}:${os_version_id}" in
          ubuntu:24.04) distro="ubuntu24" ;;
          debian:13) distro="trixie" ;;
        esac
        ;;
    esac
  fi

  if [[ "${distro}" == "unknown" ]]; then
    log "WARNING: Unrecognised OS/distro - skipping dependency install"
    return 0
  fi

  local pkg_file="${deps_dir}/packages-${distro}.txt"
  if [[ ! -f "${pkg_file}" ]]; then
    log "ERROR: Package list not found: ${pkg_file}"
    exit 1
  fi

  read_pkgs() {
    local file="$1"
    while IFS= read -r line || [[ -n "${line}" ]]; do
      line="${line%%#*}"
      line="${line#"${line%%[![:space:]]*}"}"
      line="${line%"${line##*[![:space:]]}"}"
      [[ -n "${line}" ]] && printf '%s\n' "${line}"
    done < "${file}"
  }

  local packages=()
  while IFS= read -r pkg; do
    packages+=("${pkg}")
  done < <(read_pkgs "${pkg_file}")

  if [[ "${CODE_QUALITY}" == "ON" ]] && [[ -f "${deps_dir}/packages-quality.txt" ]]; then
    while IFS= read -r pkg; do
      packages+=("${pkg}")
    done < <(read_pkgs "${deps_dir}/packages-quality.txt")
  fi

  if [[ "${ENABLE_COVERAGE}" == "ON" ]] && [[ -f "${deps_dir}/packages-coverage.txt" ]]; then
    while IFS= read -r pkg; do
      packages+=("${pkg}")
    done < <(read_pkgs "${deps_dir}/packages-coverage.txt")
  fi

  if [[ "${BUILD_SBOM}" == "ON" ]] && [[ -f "${deps_dir}/packages-sbom.txt" ]]; then
    while IFS= read -r pkg; do
      packages+=("${pkg}")
    done < <(read_pkgs "${deps_dir}/packages-sbom.txt")
  fi

  log "Installing ${#packages[@]} packages for ${distro}"

  local apt_get="apt-get"
  [[ "$(id -u)" -ne 0 ]] && apt_get="sudo apt-get"

  ${apt_get} update -qq
  DEBIAN_FRONTEND=noninteractive ${apt_get} install -y --no-install-recommends "${packages[@]}"

  if [[ "${BUILD_SBOM}" == "ON" ]] && ! command -v syft >/dev/null 2>&1; then
    local machine
    machine="$(uname -m)"
    case "${machine}" in
      x86_64|amd64|aarch64|arm64)
        log "syft not found after package install; installing via official installer"
        local install_cmd='curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh -s -- -b /usr/local/bin'
        if [[ "$(id -u)" -eq 0 ]]; then
          bash -lc "${install_cmd}"
        elif command -v sudo >/dev/null 2>&1; then
          sudo bash -lc "${install_cmd}"
        else
          log "ERROR: syft installation requires root or sudo"
          exit 1
        fi
        ;;
      *)
        log "syft auto-install unsupported on architecture ${machine}; generate SBOMs on a host runner instead"
        ;;
    esac
  fi
}

generate_sboms() {
  if ! command -v syft >/dev/null 2>&1; then
    log "ERROR: BUILD_SBOM=ON but syft is not installed"
    exit 1
  fi

  local sbom_dir
  if [[ -n "${SBOM_OUTPUT_DIR}" ]]; then
    sbom_dir="${SBOM_OUTPUT_DIR}"
  else
    sbom_dir="${BUILD_DIR}/sbom"
  fi
  mkdir -p "${sbom_dir}"

  log "Generating source SBOM"
  syft "dir:${SOURCE_DIR}" -o "spdx-json=${sbom_dir}/source.spdx.json"

  log "Generating package SBOM(s)"
  shopt -s nullglob
  local pkg
  local has_pkg="false"
  for pkg in "${BUILD_DIR}/packages"/*.deb "${BUILD_DIR}/packages"/*.tgz; do
    has_pkg="true"
    local base
    base="$(basename "${pkg}")"
    syft "file:${pkg}" -o "spdx-json=${sbom_dir}/package-${base}.spdx.json"
  done
  shopt -u nullglob

  if [[ "${has_pkg}" != "true" ]]; then
    log "ERROR: BUILD_SBOM=ON but no package artifacts were found in ${BUILD_DIR}/packages"
    exit 1
  fi

  log "SBOM output written to ${sbom_dir}"
}

if [[ "${INSTALL_DEPS}" == "ON" ]]; then
  install_deps
  log "Dependency installation complete (--install-deps); exiting without build"
  exit 0
fi

if [[ "${SBOM_ONLY}" == "ON" ]]; then
  generate_sboms
  log "SBOM generation complete (--sbom-only); exiting without build"
  exit 0
fi

if [[ "${CLEAN}" == "ON" ]]; then
  log "Cleaning build directory ${BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
fi

if [[ "${FORMAT_CHECK}" == "ON" ]]; then
  CODE_QUALITY="ON"
fi

cmake_args=(
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  -DAASDK_TEST="${BUILD_TESTS}"
)

# Pass architecture from CI env (ARCH_ID) or TARGET_ARCH to CMake
_target_arch="${TARGET_ARCH:-${ARCH_ID:-}}"
if [[ -n "${_target_arch}" ]]; then
  cmake_args+=("-DTARGET_ARCH=${_target_arch}")
fi

# Pass distro info from CI env so DebPackageFilename works inside Docker
# (lsb_release is often absent in slim images)
if [[ -n "${DISTRO_ID:-}" ]]; then
  case "${DISTRO_ID}" in
    ubuntu24)
      export DISTRO_DEB_SUITE="noble"
      export DISTRO_DEB_VERSION="ubuntu24.04"
      export DISTRO_DEB_RELEASE="ubuntu24.04"
      ;;
    trixie)
      export DISTRO_DEB_SUITE="trixie"
      export DISTRO_DEB_VERSION="deb13"
      export DISTRO_DEB_RELEASE="deb13"
      ;;
  esac
fi

if [[ "${SKIP_BUILD_PROTOBUF}" == "ON" ]]; then
  cmake_args+=("-DSKIP_BUILD_PROTOBUF=ON")
fi

if [[ "${SKIP_BUILD_ABSL}" == "ON" ]]; then
  cmake_args+=("-DSKIP_BUILD_ABSL=ON")
fi

if [[ "${ENABLE_COVERAGE}" == "ON" ]]; then
  cmake_args+=("-DCMAKE_CXX_FLAGS=--coverage" "-DCMAKE_C_FLAGS=--coverage")
fi

if [[ -n "${VERSION}" ]]; then
  cmake_args+=("-DAASDK_VERSION_OVERRIDE=${VERSION}")
else
  cmake_args+=("-UAASDK_VERSION_OVERRIDE")
fi

if [[ -n "${CMAKE_EXTRA_ARGS}" ]]; then
  # shellcheck disable=SC2206
  extra_args=( ${CMAKE_EXTRA_ARGS} )
  cmake_args+=("${extra_args[@]}")
fi

log "Configuring (${BUILD_TYPE}) from ${SOURCE_DIR} into ${BUILD_DIR}"
cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -G Ninja "${cmake_args[@]}"

if [[ "${CODE_QUALITY}" == "ON" ]]; then
  if command -v cppcheck >/dev/null 2>&1; then
    log "Running cppcheck"
    mapfile -t cppcheck_files < <(find src include protobuf \
      -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.c' -o -name '*.hpp' -o -name '*.h' \) \
      ! -name '*.ut.cpp' \
      ! -path '*/unit_test/*')

    if [[ "${#cppcheck_files[@]}" -eq 0 ]]; then
      log "No source files found for cppcheck; skipping static analysis"
    else
    cppcheck \
      --enable=warning,performance,portability,style \
      --error-exitcode=1 \
      --suppress=unusedStructMember \
      --suppress=noExplicitConstructor \
      --inline-suppr \
      --language=c++ \
      --std=c++17 \
      --quiet \
      "${cppcheck_files[@]}"
    fi
  else
    log "cppcheck not installed, skipping static analysis"
  fi

  if cmake --build "${BUILD_DIR}" --target help | grep -q "clang-format"; then
    log "Running clang-format target"
    cmake --build "${BUILD_DIR}" --target clang-format
  else
    log "clang-format target unavailable, skipping format step"
  fi

  if [[ "${FORMAT_CHECK}" == "ON" ]]; then
    log "Checking formatting diff"
    if ! git diff --exit-code -- '*.cpp' '*.cc' '*.c' '*.h' '*.hpp'; then
      log "Formatting check failed; run clang-format and commit changes"
      exit 1
    fi
  fi
fi

log "Building"
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

if [[ "${BUILD_TESTS}" == "ON" ]]; then
  log "Running tests"
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

if [[ "${BUILD_PACKAGE}" == "ON" ]]; then
  log "Packaging"
  cpack --config "${BUILD_DIR}/CPackConfig.cmake" -B "${BUILD_DIR}/packages"
fi

if [[ "${INSTALL_AFTER_BUILD}" == "ON" ]]; then
  log "Installing"
  if [[ "$(id -u)" -eq 0 ]]; then
    cmake --install "${BUILD_DIR}"
    ldconfig || true
  else
    sudo cmake --install "${BUILD_DIR}"
    sudo ldconfig || true
  fi
fi

if [[ "${BUILD_SBOM}" == "ON" ]]; then
  if [[ "${BUILD_PACKAGE}" != "ON" ]]; then
    log "ERROR: BUILD_SBOM=ON requires BUILD_PACKAGE=ON"
    exit 1
  fi
  generate_sboms
fi

log "Done"