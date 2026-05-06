#!/usr/bin/env bash
set -euo pipefail

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
log() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
err() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }
step() { echo -e "\n${BLUE}=== $1 ===${NC}\n"; }

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-build_auto}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_GUI="${BUILD_GUI:-ON}"
USE_BUNDLED_THIRD_PARTY="${USE_BUNDLED_THIRD_PARTY:-ON}"
STRICT_BUNDLED_GUI="${STRICT_BUNDLED_GUI:-1}"  # 1 = не качать imgui/implot из сети
RUN_GUI="${RUN_GUI:-0}"   # 1 = запускать GUI после CLI
CLEAN_FIRST="${CLEAN_FIRST:-1}"

cd "$PROJECT_DIR"

step "ПРОВЕРКА ПЛАТФОРМЫ"
ARCH="$(uname -m)"
OS="$(uname -s)"
log "OS: $OS, ARCH: $ARCH"
if [[ "$OS" != "Linux" ]]; then
  warn "Скрипт рассчитан на Linux (Raspberry Pi / amd64)."
fi

step "ОЧИСТКА (опционально)"
if [[ "$CLEAN_FIRST" == "1" ]]; then
  log "Удаляю: $BUILD_DIR"
  rm -rf "$BUILD_DIR"
else
  log "Очистка пропущена (CLEAN_FIRST=$CLEAN_FIRST)"
fi

step "УСТАНОВКА СИСТЕМНЫХ ЗАВИСИМОСТЕЙ"
if command -v apt >/dev/null 2>&1; then
  if [[ $EUID -eq 0 ]]; then PKG="apt"; else PKG="sudo apt"; fi
  $PKG update -qq
  $PKG install -y -qq \
    build-essential cmake pkg-config git curl wget \
    libglfw3-dev libgl1-mesa-dev mesa-common-dev
else
  warn "apt не найден. Установите вручную: cmake, компилятор C++, GLFW, OpenGL dev пакеты."
fi

step "ПРОВЕРКА third_party"
if [[ "$BUILD_GUI" == "ON" && "$USE_BUNDLED_THIRD_PARTY" == "ON" ]]; then
  if [[ -f third_party/imgui/imgui.cpp && -f third_party/implot/implot.cpp ]]; then
    log "Найдены локальные third_party/imgui и third_party/implot (будут использованы)"
  else
    if [[ "$STRICT_BUNDLED_GUI" == "1" ]]; then
      err "Не найдены third_party/imgui или third_party/implot. В strict-режиме скачивание запрещено."
    else
      warn "Не найдены локальные third_party библиотеки. CMake может использовать FetchContent."
    fi
  fi
fi

step "КОНФИГУРАЦИЯ CMAKE"
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DBUILD_GUI="$BUILD_GUI" \
  -DUSE_BUNDLED_THIRD_PARTY="$USE_BUNDLED_THIRD_PARTY"

step "СБОРКА"
cmake --build "$BUILD_DIR" -j"$(nproc)"

step "ЗАПУСК CLI БЕНЧМАРКА"
if [[ -x "$BUILD_DIR/bench_cli" ]]; then
  "$BUILD_DIR/bench_cli"
else
  err "Не найден бинарник $BUILD_DIR/bench_cli"
fi

step "ИТОГ"
log "CLI завершён. Ищите benchmark_*.csv и benchmark_*.md в корне проекта."

if [[ "$BUILD_GUI" == "ON" ]]; then
  if [[ -x "$BUILD_DIR/bench_gui" ]]; then
    if [[ "$RUN_GUI" == "1" ]]; then
      step "ЗАПУСК GUI"
      log "Запускаю GUI..."
      "$BUILD_DIR/bench_gui"
    else
      log "GUI собран: $BUILD_DIR/bench_gui"
      log "Чтобы запустить сразу из скрипта: RUN_GUI=1 ./setup_and_run.sh"
    fi
  else
    warn "GUI не собран (проверьте зависимости GLFW/OpenGL)."
  fi
fi

if [[ "$ARCH" == "x86_64" || "$ARCH" == "amd64" ]]; then
  warn "На AMD64 NEON-функции работают через scalar fallback."
  warn "Для демонстрации графиков в GUI можно включить: 'AMD64 демо-режим (эмуляция ускорения)'."
fi

log "Готово ✅"
