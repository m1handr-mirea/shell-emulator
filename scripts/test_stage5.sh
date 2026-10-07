#!/usr/bin/env bash
# Тестирование Этапа 5 (команды mkdir, mv, help и подтверждение модификации VFS ТОЛЬКО в памяти)

VFS_FILE="tests/vfs_deep.json"

echo "=== [ТЕСТ] 1. Хеш VFS-файла на физическом диске ДО запуска ==="
BEFORE_HASH=$(shasum -a 256 "$VFS_FILE" | awk '{print $1}')
echo "SHA-256: $BEFORE_HASH"

echo ""
echo "=== [ТЕСТ] 2. Запуск эмулятора с командами Этапа 5 (mkdir, mv, help) ==="
./shell_emulator --vfs "$VFS_FILE" --script scripts/startup_stage5.txt

echo ""
echo "=== [ТЕСТ] 3. Хеш VFS-файла на физическом диске ПОСЛЕ завершения эмулятора ==="
AFTER_HASH=$(shasum -a 256 "$VFS_FILE" | awk '{print $1}')
echo "SHA-256: $AFTER_HASH"

if [ "$BEFORE_HASH" == "$AFTER_HASH" ]; then
    echo ""
    echo ">>> УСПЕХ: Файл VFS на диске не изменился! Все модификации происходили исключительно в памяти."
else
    echo ""
    echo ">>> ОШИБКА: Файл на диске был изменен!"
    exit 1
fi
