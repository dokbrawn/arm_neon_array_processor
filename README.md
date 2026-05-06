# ARM NEON Array Processor

Проект под Raspberry Pi 4 (aarch64), демонстрирующий:

- скалярную и NEON-реализации обработки массива `int32_t`;
- безветвевое вычисление `abs` через `vshrq_n_s32/veorq_s32/vsubq_s32`;
- честный бенчмарк с `std::chrono::steady_clock` и прогревом;
- экспорт результатов в CSV (открывается в Excel/LibreOffice) и Markdown (можно конвертировать в PDF);
- GUI на Dear ImGui + ImPlot для визуализации.

## Алгоритм
Для каждого элемента:
- `> 0` -> добавляем как есть;
- `< 0` -> добавляем модуль;
- `== 0` -> пропускаем.

Возвращаемый тип — `int64_t`.

## Сборка CLI
```bash
g++ -O3 -march=armv8-a+simd -std=c++17 -o bench_cli main.cpp
./bench_cli
```

## Сборка через CMake (CLI + GUI)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON
cmake --build build -j
./build/bench_cli
./build/bench_gui
```

## Экспорт
После запуска `bench_cli` появятся:
- `benchmark_<timestamp>.csv` — таблица для Excel;
- `benchmark_<timestamp>.md` — текстовый отчёт.

PDF:
```bash
pandoc benchmark_<timestamp>.md -o benchmark_<timestamp>.pdf
```

## Примечания по требованиям задания
- SIMD загрузка по 4 `int32_t`: `vld1q_s32`.
- Остаток `n % 4` обрабатывается скалярно.
- Маски без ветвлений: `vcgtq_s32`, `vcltq_s32`, `vandq_s32`, `vorrq_s32`.
- Баррельный шифтер для знака: `vshrq_n_s32(vec, 31)`.
- Развёртка на 8 элементов + `__builtin_prefetch` в `process_array_neon_unrolled`.

