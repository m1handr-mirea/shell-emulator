#!/usr/bin/env bash
# Тестирование Этапа 4 (команды ls, cd, echo, cat, tail)

echo "=== [ТЕСТ] Запуск эмулятора с VFS и скриптом тестирования Этапа 4 ==="
./shell_emulator --vfs tests/vfs_deep.json --script scripts/startup_stage4.txt
echo "=== Код возврата: $? ==="
