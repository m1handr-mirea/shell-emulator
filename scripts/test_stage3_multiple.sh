#!/usr/bin/env bash
# Тестирование работы с VFS с несколькими файлами и папками (Этап 3)

VFS_FILE="tests/vfs_multiple.json"

echo "=== [ТЕСТ] Запуск с VFS с несколькими файлами: $VFS_FILE ==="
echo "Ожидаемый SHA-256 от утилиты ОС (shasum -a 256):"
shasum -a 256 "$VFS_FILE"

echo ""
echo "Вызов эмулятора с командой vfs-info:"
./shell_emulator --vfs "$VFS_FILE" << 'EOF'
vfs-info
cd etc
ls
exit
EOF
