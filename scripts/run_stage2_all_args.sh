#!/usr/bin/env bash
# Тестирование запуска со всеми параметрами (--vfs и --script)

echo "=== [ТЕСТ] Запуск со всеми параметрами (комбинация VFS и скрипта) ==="
./shell_emulator --vfs /data/configs/custom_vfs.json --script scripts/startup_stage2.txt
