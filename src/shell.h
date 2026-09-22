#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>

// Класс ShellEmulator реализует REPL и парсинг команд (Этап 1)
class ShellEmulator {
public:
    explicit ShellEmulator(const std::string& vfsName = "default_vfs");

    // Интерактивный цикл чтения команд
    void run();

    // Выполнение одной команды
    bool executeCommand(const std::string& line);

    // Разбор командной строки на аргументы с поддержкой одинарных и двойных кавычек
    static bool parseCommandLine(const std::string& line, 
                                 std::vector<std::string>& tokens, 
                                 std::string& errorMessage);

    std::string getVfsName() const { return vfsName_; }
    void setVfsName(const std::string& name) { vfsName_ = name; }

private:
    std::string vfsName_; // Имя VFS для приглашения
    bool isRunning_;     // Флаг работы REPL

    void printPrompt() const;
    static std::string trim(const std::string& str);

    // Команды-заглушки этапа 1
    void handleLs(const std::vector<std::string>& args);
    void handleCd(const std::vector<std::string>& args);
    bool handleExit(const std::vector<std::string>& args);
};

#endif // SHELL_H
