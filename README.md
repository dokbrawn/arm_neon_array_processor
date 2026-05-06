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
- Шрифт: сначала ищется Consolas, при отсутствии используется DejaVu Sans Mono.


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
