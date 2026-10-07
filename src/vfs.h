#ifndef VFS_H
#define VFS_H

#include <string>
#include <vector>
#include <map>
#include <memory>

enum class VfsNodeType {
    File,
    Directory
};

struct VfsNode {
    std::string name;
    VfsNodeType type;
    std::string content;
    std::weak_ptr<VfsNode> parent;
    std::map<std::string, std::shared_ptr<VfsNode>> children;

    VfsNode(const std::string& nodeName, VfsNodeType nodeType)
        : name(nodeName), type(nodeType) {}

    bool isDirectory() const { return type == VfsNodeType::Directory; }
    bool isFile() const { return type == VfsNodeType::File; }
};

class VirtualFileSystem {
public:
    VirtualFileSystem();

    bool loadFromJsonFile(const std::string& filePath, std::string& errorMsg);

    std::string getName() const { return vfsName_; }
    std::string getSha256() const { return sha256Hash_; }
    size_t getTotalNodes() const { return totalNodesCount_; }

    std::string getCurrentPath() const;
    std::shared_ptr<VfsNode> resolveNode(const std::string& path) const;

    std::shared_ptr<VfsNode> getCurrentDir() const { return currentDir_; }
    std::shared_ptr<VfsNode> getRoot() const { return root_; }

    bool changeDirectory(const std::string& path, std::string& errorMsg);

    // Создание каталогов в памяти (Этап 5: mkdir)
    bool makeDirectory(const std::string& path, bool createParents, std::string& errorMsg);

    // Перемещение и переименование в памяти (Этап 5: mv)
    bool moveNode(const std::string& srcPath, const std::string& destPath, std::string& errorMsg);

    void recalculateNodesCount();

private:
    std::string vfsName_;
    std::string sha256Hash_;
    size_t totalNodesCount_;
    std::shared_ptr<VfsNode> root_;
    std::shared_ptr<VfsNode> currentDir_;

    void buildNodeFromJson(const class JsonValue& jsonVal, 
                           const std::shared_ptr<VfsNode>& parentNode, 
                           const std::string& nodeName);

    size_t countNodesRecursive(const std::shared_ptr<VfsNode>& node) const;
};

#endif // VFS_H
