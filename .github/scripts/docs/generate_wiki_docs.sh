#!/usr/bin/env bash
set -euo pipefail

log() { echo "[info] $*"; }
err() { echo "[error] $*" >&2; }

ROOT_DIR="$(pwd)"
XML_DIR="${ROOT_DIR}/build/docs/xml"
MD_OUT_DIR="${ROOT_DIR}/build/docs/wiki-md"
DOXYBOOK2_VERSION="1.6.8"
DOXYBOOK2_BIN="${ROOT_DIR}/build/tools/doxybook2/doxybook2"
DOXYBOOK2_URL="https://github.com/matusnovak/doxybook2/releases/download/v${DOXYBOOK2_VERSION}/doxybook2-linux-x64.zip"

mkdir -p "${ROOT_DIR}/build/tools/doxybook2"
mkdir -p "${MD_OUT_DIR}"

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

if [ ! -x "${DOXYBOOK2_BIN}" ]; then
  log "Fetching Doxybook2 v${DOXYBOOK2_VERSION}..."
  TMP_ZIP="${ROOT_DIR}/build/tools/doxybook2/doxybook2.zip"
  curl -fsSL -o "${TMP_ZIP}" "${DOXYBOOK2_URL}"
  unzip -o "${TMP_ZIP}" -d "${ROOT_DIR}/build/tools/doxybook2" >/dev/null
  chmod +x "${DOXYBOOK2_BIN}" || true
fi

if [ ! -x "${DOXYBOOK2_BIN}" ]; then
  err "Doxybook2 binary not available at ${DOXYBOOK2_BIN}. Aborting."
  exit 1
fi

log "Converting XML to Markdown via Doxybook2..."
"${DOXYBOOK2_BIN}" \
  --input "${XML_DIR}" \
  --output "${MD_OUT_DIR}" \
  --config "${ROOT_DIR}/docs/doxybook/config.json"

log "Markdown generated at ${MD_OUT_DIR}"
