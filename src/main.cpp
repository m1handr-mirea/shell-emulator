#include "shell.h"
#include <iostream>
#include <string>

void printHelp(const char* progName) {
    std::cout << "Использование: " << progName << " [ПАРАМЕТРЫ]\n"
              << "Эмулятор командной строки UNIX-подобной ОС.\n\n"
              << "Параметры:\n"
              << "  -v, --vfs <путь>      Физический путь к образу виртуальной файловой системы (VFS)\n"
              << "  -s, --script <путь>   Путь к стартовому скрипту с командами для выполнения\n"
              << "  -h, --help            Показать эту справку и выйти\n\n"
              << "Пример запуска:\n"
              << "  " << progName << " --vfs test_vfs.json --script startup.txt\n";
}

int main(int argc, char* argv[]) {
    ShellConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--vfs") {
            if (i + 1 < argc) {
                config.vfsPath = argv[++i];
            } else {
                std::cerr << "Ошибка: ключ '" << arg << "' требует указания пути к VFS." << std::endl;
                return 1;
            }
        } else if (arg == "-s" || arg == "--script") {
            if (i + 1 < argc) {
                config.scriptPath = argv[++i];
            } else {
                std::cerr << "Ошибка: ключ '" << arg << "' требует указания пути к скрипту." << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Ошибка: неизвестный параметр '" << arg << "'. Используйте --help для справки." << std::endl;
            return 1;
        }
    }

    try {
        ShellEmulator shell(config);
        shell.run();
    } catch (const std::exception& ex) {
        std::cerr << "Критическая ошибка: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
