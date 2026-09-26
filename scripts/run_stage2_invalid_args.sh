#!/usr/bin/env bash
# Тестирование некорректных аргументов командной строки и справки

echo "=== [ТЕСТ] Запуск с флагом справки --help ==="
./shell_emulator --help

echo ""
echo "=== [ТЕСТ] Запуск с неизвестным параметром --unknown ==="
./shell_emulator --unknown-flag || true

echo ""
echo "=== [ТЕСТ] Запуск с отсутствующим значением для --vfs ==="
./shell_emulator --vfs || true
