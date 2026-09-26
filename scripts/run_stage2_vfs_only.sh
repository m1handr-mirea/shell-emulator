#!/usr/bin/env bash
# Тестирование параметров пути к VFS (--vfs и -v)

echo "=== [ТЕСТ] Запуск с длинным флагом --vfs ==="
./shell_emulator --vfs /path/to/my_vfs.json << 'EOF'
ls
exit
EOF

echo ""
echo "=== [ТЕСТ] Запуск с коротким флагом -v ==="
./shell_emulator -v ./another_vfs.json << 'EOF'
cd /home
exit
EOF
