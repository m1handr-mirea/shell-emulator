#!/usr/bin/env bash
# Демонстрационный скрипт для Этапа 1 (REPL, парсинг, заглушки)

echo "=== Сборка проекта ==="
make

echo ""
echo "=== Тестирование интерактивного режима (Этап 1) ==="
./shell_emulator << 'EOF'
ls
ls -l -a
ls "folder with spaces" 'another quoted argument'
cd
cd "my documents"
cd extra arg1 arg2
unknown_command
echo "this command is not yet implemented"
ls "unclosed quote
exit
EOF

echo ""
echo "=== Тестирование завершено ==="
