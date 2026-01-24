#!/usr/bin/env bash
# Project: Crankshaft
# This file is part of Crankshaft project.
# Copyright (C) 2025 OpenCarDev Team
#
#  Crankshaft is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  Crankshaft is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Crankshaft. If not, see <http://www.gnu.org/licenses/>.

set -euo pipefail

# British English messaging
log() { echo "[info] $*"; }
warn() { echo "[warn] $*" >&2; }
err() { echo "[error] $*" >&2; }

ROOT_DIR="$(pwd)"
XML_DIR="${ROOT_DIR}/build/docs/xml"
MD_OUT_DIR="${ROOT_DIR}/build/docs/wiki-md"
DOXYBOOK2_VERSION="1.5.0"
DOXYBOOK2_BIN="${ROOT_DIR}/build/tools/doxybook2/bin/doxybook2"
DOXYBOOK2_URL="https://github.com/matusnovak/doxybook2/releases/download/v${DOXYBOOK2_VERSION}/doxybook2-linux-amd64-v${DOXYBOOK2_VERSION}.zip"
DOXYBOOK2_SHA256="3fb90354b7ab3e8139a5606221865ff6aa0c53f2805e56088dcbd8185ebb5b41"

mkdir -p "${ROOT_DIR}/build/tools/doxybook2"
mkdir -p "${MD_OUT_DIR}"
# Ensure common output categories exist to avoid renderer write failures
mkdir -p "${MD_OUT_DIR}/Namespaces" "${MD_OUT_DIR}/Classes" "${MD_OUT_DIR}/Files" "${MD_OUT_DIR}/Modules" "${MD_OUT_DIR}/Pages"

log "Running Doxygen to generate XML..."
if [ ! -f "${ROOT_DIR}/Doxyfile" ]; then
  err "Doxyfile not found at repo root. Aborting."
  exit 1
fi

doxygen "${ROOT_DIR}/Doxyfile"

if [ ! -d "${XML_DIR}" ]; then
  err "Doxygen XML output not found at ${XML_DIR}. Aborting."
  exit 1
fi

# Mirror XML directory structure under Namespaces to avoid nested path creation errors
while IFS= read -r subdir; do
  mkdir -p "${MD_OUT_DIR}/Namespaces/${subdir}"
done < <(cd "${XML_DIR}" && find . -type d -printf '%P\n' | sed '/^$/d')

if [ ! -x "${DOXYBOOK2_BIN}" ]; then
  log "Fetching Doxybook2 v${DOXYBOOK2_VERSION}..."
  TMP_ZIP="${ROOT_DIR}/build/tools/doxybook2/doxybook2.zip"
  
  log "Downloading from: ${DOXYBOOK2_URL}"
  if ! curl -fsSL -o "${TMP_ZIP}" "${DOXYBOOK2_URL}"; then
    err "Failed to download doxybook2"
    exit 1
  fi
  
  if [ ! -f "${TMP_ZIP}" ]; then
    err "Downloaded file not found: ${TMP_ZIP}"
    exit 1
  fi
  
  TMP_ZIP_SIZE=$(wc -c < "${TMP_ZIP}")
  log "Downloaded file size: ${TMP_ZIP_SIZE} bytes"
  
  # Verify SHA256 checksum to prevent supply-chain attacks
  log "Verifying SHA256 checksum..."
  ACTUAL_SHA256=$(sha256sum "${TMP_ZIP}" | cut -d' ' -f1)
  if [ "${ACTUAL_SHA256}" != "${DOXYBOOK2_SHA256}" ]; then
    err "SHA256 checksum mismatch for doxybook2!"
    err "Expected: ${DOXYBOOK2_SHA256}"
    err "Actual:   ${ACTUAL_SHA256}"
    rm -f "${TMP_ZIP}"
    exit 1
  fi
  log "Checksum verified successfully"
  
  log "Extracting doxybook2..."
  if ! unzip -o "${TMP_ZIP}" -d "${ROOT_DIR}/build/tools/doxybook2"; then
    err "Failed to extract doxybook2"
    rm -f "${TMP_ZIP}"
    exit 1
  fi
  
  # List extracted files for debugging
  log "Extracted files:"
  find "${ROOT_DIR}/build/tools/doxybook2" -type f -exec ls -la {} \;
  
  # Make binary executable
  if [ -f "${DOXYBOOK2_BIN}" ]; then
    chmod +x "${DOXYBOOK2_BIN}"
    log "Binary is now executable"
  else
    err "Binary not found after extraction at ${DOXYBOOK2_BIN}"
    log "Directory contents:"
    find "${ROOT_DIR}/build/tools/doxybook2" -type f
    exit 1
  fi
  
  rm -f "${TMP_ZIP}"
fi

if [ ! -x "${DOXYBOOK2_BIN}" ]; then
  err "Doxybook2 binary not available at ${DOXYBOOK2_BIN}. Aborting."
  log "Directory contents:"
  find "${ROOT_DIR}/build/tools/doxybook2" -type f 2>/dev/null || true
  exit 1
fi

log "Converting XML to Markdown via Doxybook2..."
"${DOXYBOOK2_BIN}" \
  --input "${XML_DIR}" \
  --output "${MD_OUT_DIR}" \
  --config "${ROOT_DIR}/docs/doxybook/config.json"

log "Markdown generated at ${MD_OUT_DIR}"