# ARM NEON Array Processor

Проект под Raspberry Pi 4 (aarch64), демонстрирующий:
- скалярную и NEON-реализации обработки массива `int32_t`;
- безветвевое вычисление `abs` через `vshrq_n_s32/veorq_s32/vsubq_s32`;
- корректный бенчмарк с `std::chrono::steady_clock` и прогревом;
- экспорт в CSV (Excel/LibreOffice) и Markdown (дальше в PDF);
- GUI на Dear ImGui + ImPlot.

## Что должно лежать в `third_party`
Если хотите оффлайн сборку GUI без скачивания:
- `third_party/imgui/...` (файлы `imgui.cpp`, `imgui_draw.cpp`, `backends/...`)
- `third_party/implot/...` (файлы `implot.cpp`, `implot_items.cpp`)

Если папок нет, CMake попробует скачать зависимости автоматически (FetchContent).

## Быстрый запуск CLI
```bash
g++ -O3 -march=armv8-a+simd -std=c++17 -o bench_cli main.cpp
./bench_cli
```

## Сборка CMake (CLI + GUI)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DUSE_BUNDLED_THIRD_PARTY=ON
cmake --build build -j
./build/bench_cli
./build/bench_gui
```

## Как тестировать на Raspberry Pi 4 (aarch64)

### 1) Проверка окружения
```bash
uname -m
lscpu | grep -E 'Architecture|Model name'
g++ --version
```
Ожидаем `aarch64` и ARM Cortex-A72 (или совместимый).

### 2) Установка пакетов
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libglfw3-dev libgl1-mesa-dev mesa-common-dev
# для PDF-экспорта:
sudo apt install -y pandoc
```

### 3) Сборка
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DUSE_BUNDLED_THIRD_PARTY=ON
cmake --build build -j4
```

### 4) Запуск и проверка корректности
```bash
./build/bench_cli
```
- В начале выполняется `test_correctness()` (scalar == neon == unrolled).
- На выходе создаются `benchmark_<timestamp>.csv` и `benchmark_<timestamp>.md`.

### 5) Проверка ускорения
Откройте CSV и сравните колонки `scalar_ms` vs `neon_ms`/`neon_unrolled_ms`.
Для вашей цели смотрите массивы `>= 1000` и больше.

### 6) Проверка GUI
```bash
./build/bench_gui
```
Если запускаете по SSH, нужен X11-forwarding или локальный запуск с рабочим столом.

### 7) PDF отчет
```bash
pandoc benchmark_<timestamp>.md -o benchmark_<timestamp>.pdf
```

## Примечания по заданию
- SIMD загрузка по 4 `int32_t`: `vld1q_s32`.
- Остаток `n % 4`: скалярно.
- Условная логика без ветвлений: маски + `vand/vorr`.
- Баррельный шифтер: `vshrq_n_s32(vec, 31)`.
- Дополнительно: развёртка на 8 элементов + `__builtin_prefetch`.

## GUI возможности
- Полностью русскоязычный интерфейс.
- Настраиваемые параметры теста: стартовый/конечный размер, число точек, прогрев, итерации, seed.
- График времени (линии) и столбчатая диаграмма ускорения.
- Переключатели отображения графиков и логарифмической оси X.
- Сравнение scalar/NEON/unrolled в таблице.
- Кнопка сохранения CSV прямо из GUI.
- Шрифт на Linux/Raspberry Pi: приоритет Noto Sans Mono / Ubuntu Mono / DejaVu Sans Mono (Consolas только как опциональный последний fallback).


## Тестирование на AMD64 (для проверки интерфейса)
На x86_64/amd64 NEON-интринсики не используются, поэтому `process_array_neon*` переходят в scalar-fallback.
Это нормально для проверки UI/графиков.

```bash
cmake -S . -B build_amd64 -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DUSE_BUNDLED_THIRD_PARTY=ON
cmake --build build_amd64 -j
./build_amd64/bench_cli
./build_amd64/bench_gui
```

В GUI можно включить пункт **"AMD64 демо-режим (эмуляция ускорения)"**, чтобы увидеть более наглядные графики ускорения для демонстрации интерфейса.


## Windows + vcpkg (AMD64)
Если у вас `C:\vcpkg`, для GUI нужен glfw3 через toolchain vcpkg.

```powershell
# один раз
C:\vcpkg\vcpkg install glfw3:x64-windows

# конфигурация
cmake -S . -B build_amd64 -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON -DUSE_BUNDLED_THIRD_PARTY=ON

# сборка
cmake --build build_amd64 --config Release

# запуск
.\build_amd64\Release\bench_cli.exe
.\build_amd64\Release\bench_gui.exe
```

Если не нужен GUI, можно собрать только CLI: `-DBUILD_GUI=OFF`.


## Быстрый авто-скрипт (Linux / Raspberry Pi / AMD64)
В корне есть `setup_and_run.sh`, который:
1. ставит системные зависимости (через `apt`),
2. конфигурирует и собирает проект,
3. запускает `bench_cli`,
4. опционально запускает GUI.

```bash
chmod +x setup_and_run.sh
./setup_and_run.sh
```

Полезные параметры:
```bash
BUILD_GUI=ON RUN_GUI=1 ./setup_and_run.sh
BUILD_GUI=OFF ./setup_and_run.sh
CLEAN_FIRST=0 BUILD_DIR=build_custom ./setup_and_run.sh
```


### Частые ошибки на Windows
1. **"generator platform x64 does not match previous"**  
   Удалите кэш сборки перед повторной конфигурацией:
   ```powershell
   Remove-Item -Recurse -Force .\build_amd64
   ```

2. **`vcpkg` не найден в PATH**  
   Используйте полный путь:
   ```powershell
   C:\vcpkg\vcpkg.exe install glfw3:x64-windows
   ```

3. **Нет `GLFW/glfw3.h` при сборке GUI**  
   Проверьте, что CMake запускается с toolchain:
   `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`.

4. Если конфигурация упала, `bench_cli.exe`/`bench_gui.exe` не появятся — сначала исправьте ошибку configure/build.


Примечание: логарифмическая ось X зависит от версии ImPlot; в старых версиях может работать как обычная ось без ошибки сборки.


MSVC кодировка: для корректного русского текста в GUI в CMake включён флаг `/utf-8`, а на Windows шрифт выбирается из `Segoe UI/Arial/Consolas`.


В GUI графики теперь не фиксируются авто-подгонкой на каждом кадре: их можно двигать мышью (pan) и масштабировать колесом (zoom).


Для Raspberry Pi (Ubuntu) без скачивания ImGui/ImPlot: используйте `BUILD_GUI=ON USE_BUNDLED_THIRD_PARTY=ON STRICT_BUNDLED_GUI=1 ./setup_and_run.sh` (по умолчанию strict уже включён).
