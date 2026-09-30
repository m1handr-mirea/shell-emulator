#!/usr/bin/env bash
# Тестирование работы с минимальной VFS (Этап 3)

VFS_FILE="tests/vfs_minimal.json"

echo "=== [ТЕСТ] Запуск с минимальной VFS: $VFS_FILE ==="
echo "Ожидаемый SHA-256 от утилиты ОС (shasum -a 256):"
shasum -a 256 "$VFS_FILE"

echo ""
echo "Вызов эмулятора с командой vfs-info:"
./shell_emulator --vfs "$VFS_FILE" << 'EOF'
vfs-info
ls
exit
EOF
