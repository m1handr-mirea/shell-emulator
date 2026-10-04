#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>
#include <memory>
#include "vfs.h"

struct ShellConfig {
    std::string vfsPath;     // Физический путь к VFS
    std::string scriptPath;  // Путь к стартовому скрипту команд
};

class ShellEmulator {
public:
    explicit ShellEmulator(const ShellConfig& config = ShellConfig());

    void run();
    bool runScript(const std::string& scriptPath);
    bool executeCommand(const std::string& line);

    static bool parseCommandLine(const std::string& line, 
                                 std::vector<std::string>& tokens, 
                                 std::string& errorMessage);

    std::string getVfsName() const { return vfsName_; }
    void setVfsName(const std::string& name) { vfsName_ = name; }
    const ShellConfig& getConfig() const { return config_; }
    VirtualFileSystem& getVfs() { return vfs_; }

private:
    ShellConfig config_;
    std::string vfsName_;
    VirtualFileSystem vfs_;
    bool isRunning_;

    void printDebugConfig() const;
    void printPrompt() const;
    static std::string trim(const std::string& str);
    static bool isCppComment(const std::string& line);

    void handleVfsInfo(const std::vector<std::string>& args);
    void handleLs(const std::vector<std::string>& args);
    void handleCd(const std::vector<std::string>& args);
    void handleEcho(const std::vector<std::string>& args);
    void handleCat(const std::vector<std::string>& args);
    void handleTail(const std::vector<std::string>& args);
    bool handleExit(const std::vector<std::string>& args);
};

#endif // SHELL_H
