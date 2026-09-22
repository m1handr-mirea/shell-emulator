#include "shell.h"
#include <iostream>

int main() {
    try {
        ShellEmulator shell("default_vfs");
        shell.run();
    } catch (const std::exception& ex) {
        std::cerr << "Критическая ошибка: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
