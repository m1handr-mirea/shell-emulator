#include "shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

ShellEmulator::ShellEmulator(const ShellConfig& config)
    : config_(config), vfsName_("default_vfs"), isRunning_(false) {}

std::string ShellEmulator::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool ShellEmulator::isCppComment(const std::string& line) {
    std::string trimmed = trim(line);
    return (trimmed.rfind("//", 0) == 0);
}

void ShellEmulator::printDebugConfig() const {
    std::cout << "================== [DEBUG] НАСТРОЙКИ ЭМУЛЯТОРА ==================" << std::endl;
    std::cout << "  Физический путь к VFS: " 
              << (config_.vfsPath.empty() ? "(не задан)" : config_.vfsPath) << std::endl;
    std::cout << "  Путь к стартовому скрипту: " 
              << (config_.scriptPath.empty() ? "(не задан)" : config_.scriptPath) << std::endl;
    std::cout << "==================================================================" << std::endl;
}

void ShellEmulator::printPrompt() const {
    std::cout << "[" << vfsName_ << ":" << vfs_.getCurrentPath() << "]$ " << std::flush;
}

bool ShellEmulator::parseCommandLine(const std::string& line, 
                                     std::vector<std::string>& tokens, 
                                     std::string& errorMessage) {
    tokens.clear();
    errorMessage.clear();

    std::string currentToken;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool tokenStarted = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '\\' && !inSingleQuote && (i + 1 < line.length())) {
            char nextC = line[i + 1];
            if (nextC == '"' || nextC == '\\' || nextC == ' ') {
                currentToken += nextC;
                tokenStarted = true;
                ++i;
                continue;
            }
        }

        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            tokenStarted = true;
        } else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            tokenStarted = true;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !inSingleQuote && !inDoubleQuote) {
            if (tokenStarted) {
                tokens.push_back(currentToken);
                currentToken.clear();
                tokenStarted = false;
            }
        } else {
            currentToken += c;
            tokenStarted = true;
        }
    }

    if (inSingleQuote) {
        errorMessage = "Синтаксическая ошибка: незакрытая одинарная кавычка (') в команде";
        return false;
    }
    if (inDoubleQuote) {
        errorMessage = "Синтаксическая ошибка: незакрытая двойная кавычка (\") в команде";
        return false;
    }

    if (tokenStarted) {
        tokens.push_back(currentToken);
    }

    return true;
}

void ShellEmulator::handleVfsInfo(const std::vector<std::string>& args) {
    if (!args.empty()) {
        std::cerr << "vfs-info: команда не принимает аргументов" << std::endl;
        return;
    }

    std::cout << "--- Информация о VFS ---" << std::endl;
    std::cout << "  Имя VFS: " << vfs_.getName() << std::endl;
    std::string hash = vfs_.getSha256();
    if (hash.empty()) {
        std::cout << "  SHA-256 данных: (VFS не была загружена из внешнего файла, "
                  << "используется образ по умолчанию)" << std::endl;
    } else {
        std::cout << "  SHA-256 данных: " << hash << std::endl;
    }
    std::cout << "  Всего файлов и каталогов в памяти: " << vfs_.getTotalNodes() << std::endl;
    std::cout << "  Текущий рабочий каталог: " << vfs_.getCurrentPath() << std::endl;
}

void ShellEmulator::handleLs(const std::vector<std::string>& args) {
    std::vector<std::string> paths;
    for (const auto& arg : args) {
        if (!arg.empty() && arg[0] != '-') {
            paths.push_back(arg);
        }
    }

    if (paths.empty()) {
        auto currentDir = vfs_.getCurrentDir();
        if (!currentDir) return;

        for (const auto& entry : currentDir->children) {
            if (entry.second->isDirectory()) {
                std::cout << entry.first << "/  ";
            } else {
                std::cout << entry.first << "  ";
            }
        }
        std::cout << std::endl;
        return;
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        const std::string& path = paths[i];
        if (paths.size() > 1) {
            std::cout << path << ":" << std::endl;
        }

        auto node = vfs_.resolveNode(path);
        if (!node) {
            std::cerr << "ls: cannot access '" << path << "': No such file or directory" << std::endl;
            continue;
        }

        if (node->isDirectory()) {
            for (const auto& entry : node->children) {
                if (entry.second->isDirectory()) {
                    std::cout << entry.first << "/  ";
                } else {
                    std::cout << entry.first << "  ";
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << node->name << std::endl;
        }

        if (paths.size() > 1 && i + 1 < paths.size()) {
            std::cout << std::endl;
        }
    }
}

void ShellEmulator::handleCd(const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::cerr << "cd: слишком много аргументов (ожидается не более 1 пути)" << std::endl;
        return;
    }

    std::string targetPath = args.empty() ? "/" : args[0];
    std::string errorMsg;
    if (!vfs_.changeDirectory(targetPath, errorMsg)) {
        std::cerr << errorMsg << std::endl;
    }
}

void ShellEmulator::handleEcho(const std::vector<std::string>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i] << (i + 1 < args.size() ? " " : "");
    }
    std::cout << std::endl;
}

void ShellEmulator::handleCat(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "cat: пропущен операнд (укажите имя файла)" << std::endl;
        return;
    }

    for (const auto& filePath : args) {
        auto node = vfs_.resolveNode(filePath);
        if (!node) {
            std::cerr << "cat: " << filePath << ": No such file or directory" << std::endl;
            continue;
        }

        if (node->isDirectory()) {
            std::cerr << "cat: " << filePath << ": Is a directory" << std::endl;
            continue;
        }

        std::cout << node->content;
        if (!node->content.empty() && node->content.back() != '\n') {
            std::cout << std::endl;
        }
    }
}

void ShellEmulator::handleTail(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "tail: пропущен операнд (укажите имя файла)" << std::endl;
        return;
    }

    int linesCount = 10;
    std::string targetFile;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "-n" && i + 1 < args.size()) {
            try {
                linesCount = std::stoi(args[++i]);
                if (linesCount < 0) linesCount = 10;
            } catch (...) {
                std::cerr << "tail: неверное количество строк: '" << args[i] << "'" << std::endl;
                return;
            }
        } else if (arg.rfind("-n", 0) == 0 && arg.size() > 2) {
            try {
                linesCount = std::stoi(arg.substr(2));
                if (linesCount < 0) linesCount = 10;
            } catch (...) {
                std::cerr << "tail: неверное количество строк: '" << arg << "'" << std::endl;
                return;
            }
        } else {
            targetFile = arg;
        }
    }

    if (targetFile.empty()) {
        std::cerr << "tail: пропущен операнд (укажите имя файла)" << std::endl;
        return;
    }

    auto node = vfs_.resolveNode(targetFile);
    if (!node) {
        std::cerr << "tail: cannot open '" << targetFile << "' for reading: No such file or directory" << std::endl;
        return;
    }

    if (node->isDirectory()) {
        std::cerr << "tail: error reading '" << targetFile << "': Is a directory" << std::endl;
        return;
    }

    std::vector<std::string> lines;
    std::stringstream ss(node->content);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    size_t start = 0;
    if (lines.size() > static_cast<size_t>(linesCount)) {
        start = lines.size() - static_cast<size_t>(linesCount);
    }

    for (size_t i = start; i < lines.size(); ++i) {
        std::cout << lines[i] << std::endl;
    }
}

void ShellEmulator::handleMkdir(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "mkdir: пропущен операнд (укажите путь к каталогу)" << std::endl;
        return;
    }

    bool createParents = false;
    std::vector<std::string> paths;

    for (const auto& arg : args) {
        if (arg == "-p") {
            createParents = true;
        } else if (arg.rfind("-", 0) == 0) {
            std::cerr << "mkdir: неизвестный ключ '" << arg << "'" << std::endl;
            return;
        } else {
            paths.push_back(arg);
        }
    }

    if (paths.empty()) {
        std::cerr << "mkdir: пропущен операнд" << std::endl;
        return;
    }

    for (const auto& path : paths) {
        std::string err;
        if (!vfs_.makeDirectory(path, createParents, err)) {
            std::cerr << err << std::endl;
        }
    }
}

void ShellEmulator::handleMv(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        if (args.empty()) {
            std::cerr << "mv: пропущен операнд файла" << std::endl;
        } else {
            std::cerr << "mv: пропущен целевой операнд после '" << args[0] << "'" << std::endl;
        }
        return;
    }

    if (args.size() > 2) {
        std::cerr << "mv: перемещение нескольких файлов за раз не поддерживается" << std::endl;
        return;
    }

    const std::string& src = args[0];
    const std::string& dest = args[1];

    std::string err;
    if (!vfs_.moveNode(src, dest, err)) {
        std::cerr << err << std::endl;
    }
}

void ShellEmulator::handleHelp(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Доступные команды эмулятора:\n"
                  << "  ls [ПУТЬ...]          Вывести список файлов и папок\n"
                  << "  cd [ПУТЬ]             Сменить текущий рабочий каталог\n"
                  << "  echo [АРГУМЕНТЫ...]   Вывести строку в консоль\n"
                  << "  cat <ФАЙЛ...>         Вывести содержимое файлов\n"
                  << "  tail [-n N] <ФАЙЛ>    Вывести последние N строк файла (по умолчанию 10)\n"
                  << "  mkdir [-p] <КАТАЛОГ>  Создать каталог в памяти VFS (-p для промежуточных)\n"
                  << "  mv <ИСТОЧНИК> <ЦЕЛЬ>  Переместить или переименовать файл/папку в VFS\n"
                  << "  vfs-info              Информация о VFS (имя, SHA-256, число объектов)\n"
                  << "  help [КОМАНДА]        Показать справку по командам\n"
                  << "  exit                  Завершить работу эмулятора" << std::endl;
        return;
    }

    const std::string& cmd = args[0];
    if (cmd == "ls") {
        std::cout << "ls [ПУТЬ...]\n  Выводит список файлов и директорий. Директории помечаются '/'.\n";
    } else if (cmd == "cd") {
        std::cout << "cd [ПУТЬ]\n  Переходит в указанную директорию. Поддерживает '.', '..', '/'.\n";
    } else if (cmd == "echo") {
        std::cout << "echo [АРГУМЕНТЫ...]\n  Печатает аргументы через пробел с сохранением кавычек.\n";
    } else if (cmd == "cat") {
        std::cout << "cat <ФАЙЛ...>\n  Выводит текстовое или декодированное Base64 содержимое файлов.\n";
    } else if (cmd == "tail") {
        std::cout << "tail [-n КОЛИЧЕСТВО] <ФАЙЛ>\n  Выводит последние строки файла (по умолчанию 10).\n";
    } else if (cmd == "mkdir") {
        std::cout << "mkdir [-p] <ПУТЬ...>\n  Создает каталог в памяти VFS. Флаг -p создает промежуточные каталоги.\n";
    } else if (cmd == "mv") {
        std::cout << "mv <ИСТОЧНИК> <НАЗНАЧЕНИЕ>\n  Перемещает или переименовывает файлы и каталоги в памяти VFS.\n";
    } else if (cmd == "vfs-info") {
        std::cout << "vfs-info\n  Выводит имя VFS, SHA-256 хеш данных и общее количество узлов в памяти.\n";
    } else if (cmd == "help") {
        std::cout << "help [КОМАНДА]\n  Отображает список всех команд или справку по заданной команде.\n";
    } else if (cmd == "exit") {
        std::cout << "exit\n  Завершает работу эмулятора командной строки.\n";
    } else {
        std::cerr << "help: неизвестная команда '" << cmd << "'. Введите help для списка всех команд." << std::endl;
    }
}

bool ShellEmulator::handleExit(const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::cerr << "exit: слишком много аргументов" << std::endl;
        return true;
    }
    std::cout << "Выход из эмулятора командной строки..." << std::endl;
    return false;
}

bool ShellEmulator::executeCommand(const std::string& line) {
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) {
        return true;
    }

    std::vector<std::string> tokens;
    std::string errorMsg;
    if (!parseCommandLine(trimmedLine, tokens, errorMsg)) {
        std::cerr << "shell: " << errorMsg << std::endl;
        return true;
    }

    if (tokens.empty()) {
        return true;
    }

    const std::string& command = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    if (command == "exit") {
        return handleExit(args);
    } else if (command == "vfs-info") {
        handleVfsInfo(args);
    } else if (command == "ls") {
        handleLs(args);
    } else if (command == "cd") {
        handleCd(args);
    } else if (command == "echo") {
        handleEcho(args);
    } else if (command == "cat") {
        handleCat(args);
    } else if (command == "tail") {
        handleTail(args);
    } else if (command == "mkdir") {
        handleMkdir(args);
    } else if (command == "mv") {
        handleMv(args);
    } else if (command == "help") {
        handleHelp(args);
    } else {
        std::cerr << "shell: неизвестная команда: '" << command 
                  << "'. Введите help для списка доступных команд." << std::endl;
    }

    return true;
}

bool ShellEmulator::runScript(const std::string& scriptPath) {
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        std::cerr << "shell: ошибка открытия стартового скрипта '" << scriptPath << "'" << std::endl;
        return false;
    }

    std::cout << "[INFO] Начало выполнения стартового скрипта: " << scriptPath << std::endl;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        if (isCppComment(trimmed)) {
            std::cout << trimmed << std::endl;
            continue;
        }

        printPrompt();
        std::cout << trimmed << std::endl;

        if (!executeCommand(trimmed)) {
            std::cout << "[INFO] Скрипт вызвал завершение работы (exit)." << std::endl;
            return false;
        }
    }

    std::cout << "[INFO] Стартовый скрипт успешно завершен." << std::endl;
    return true;
}

void ShellEmulator::run() {
    printDebugConfig();

    if (!config_.vfsPath.empty()) {
        std::string err;
        if (!vfs_.loadFromJsonFile(config_.vfsPath, err)) {
            std::cerr << "shell: " << err << std::endl;
        } else {
            vfsName_ = vfs_.getName();
            std::cout << "[INFO] Виртуальная файловая система '" << vfsName_ 
                      << "' успешно загружена в память." << std::endl;
        }
    }

    if (!config_.scriptPath.empty()) {
        bool continueRepl = runScript(config_.scriptPath);
        if (!continueRepl) {
            return;
        }
    }

    isRunning_ = true;
    std::string inputLine;

    std::cout << "Эмулятор оболочки ОС готов к работе. Введите 'help' для справки или 'exit' для выхода." << std::endl;

    while (isRunning_) {
        printPrompt();
        if (!std::getline(std::cin, inputLine)) {
            std::cout << std::endl;
            break;
        }

        if (!executeCommand(inputLine)) {
            isRunning_ = false;
        }
    }
}
