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
    std::cout << "[" << vfsName_ << "]$ " << std::flush;
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

void ShellEmulator::handleLs(const std::vector<std::string>& args) {
    std::cout << "[STUB ls] Вызвана команда ls. ";
    if (args.empty()) {
        std::cout << "Аргументы отсутствуют." << std::endl;
    } else {
        std::cout << "Аргументы (" << args.size() << "): ";
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << "[" << args[i] << "]" << (i + 1 < args.size() ? ", " : "");
        }
        std::cout << std::endl;
    }
}

void ShellEmulator::handleCd(const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::cerr << "cd: слишком много аргументов (ожидается не более 1 пути)" << std::endl;
        return;
    }

    std::cout << "[STUB cd] Вызвана команда cd. ";
    if (args.empty()) {
        std::cout << "Переход в домашнюю директорию (по умолчанию /)" << std::endl;
    } else {
        std::cout << "Целевой путь: [" << args[0] << "]" << std::endl;
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
    } else if (command == "ls") {
        handleLs(args);
    } else if (command == "cd") {
        handleCd(args);
    } else {
        std::cerr << "shell: неизвестная команда: '" << command 
                  << "'. Доступные команды: ls, cd, exit." << std::endl;
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

    if (!config_.scriptPath.empty()) {
        bool continueRepl = runScript(config_.scriptPath);
        if (!continueRepl) {
            return;
        }
    }

    isRunning_ = true;
    std::string inputLine;

    std::cout << "Эмулятор оболочки ОС готов к работе. Введите команду или 'exit'." << std::endl;

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
