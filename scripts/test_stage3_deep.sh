#!/usr/bin/env bash
# Тестирование работы с глубокой VFS (>= 3 уровней вложенности) (Этап 3)

VFS_FILE="tests/vfs_deep.json"

echo "=== [ТЕСТ] Запуск с глубокой VFS (>= 3 уровней): $VFS_FILE ==="
echo "Ожидаемый SHA-256 от утилиты ОС (shasum -a 256):"
shasum -a 256 "$VFS_FILE"

echo ""
echo "Вызов эмулятора с командой vfs-info:"
./shell_emulator --vfs "$VFS_FILE" << 'EOF'
vfs-info
cd home/student/projects/shell_emulator
ls -la "code.cpp"
exit
EOF
