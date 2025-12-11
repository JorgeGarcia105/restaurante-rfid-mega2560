#!/usr/bin/env bash
set -euo pipefail
# upload.sh - compila el sketch, hace commit/push y sube al Arduino (usa arduino-cli)
# Uso: ./upload.sh [PORT] ["commit message"] [tag]
# Ejemplo: ./upload.sh COM3 "release v0.1.1" v0.1.1

PORT=${1:-COM3}
MSG=${2:-"chore: build and deploy"}
TAG=${3:-""}
FQBN="arduino:avr:mega"

SKETCH_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SKETCH_DIR"

echo "[upload.sh] Directorio: $SKETCH_DIR"
echo "[upload.sh] Compilando (fqbn=$FQBN)..."
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "ERROR: arduino-cli no encontrado en PATH." >&2
  exit 2
fi

arduino-cli compile --fqbn "$FQBN" .
echo "[upload.sh] Compilación OK."

# Git: añadir, commit y push
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git add .
  # commit sólo si hay cambios
  if ! git diff --cached --quiet; then
    git commit -m "$MSG" || true
  else
    echo "[upload.sh] No hay cambios para commitear."
  fi
  # push main branch
  if git rev-parse --abbrev-ref HEAD >/dev/null 2>&1; then
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
  else
    BRANCH="main"
  fi
  git push origin "$BRANCH" || echo "[upload.sh] Warning: no se pudo hacer push (¿remote configurado?)."
  if [ -n "$TAG" ]; then
    git tag -a "$TAG" -m "$MSG" || true
    git push origin --tags || echo "[upload.sh] Warning: no se pudo pushear tags."
  fi
else
  echo "[upload.sh] No es un repositorio git. Saltando commit/push."
fi

echo "[upload.sh] Subiendo al Arduino en puerto $PORT..."
arduino-cli upload -p "$PORT" --fqbn "$FQBN" .
echo "[upload.sh] Upload finalizado."

echo "[upload.sh] Nota: asegúrate de tener el puerto correcto y permisos sobre el dispositivo."
