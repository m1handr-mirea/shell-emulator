#!/usr/bin/env bash
# Тестирование запуска со стартовым скриптом (--script и -s)

echo "=== [ТЕСТ] Запуск с длинным флагом --script ==="
./shell_emulator --script scripts/startup_stage2.txt

echo ""
echo "=== [ТЕСТ] Запуск с коротким флагом -s ==="
./shell_emulator -s scripts/startup_stage2.txt
