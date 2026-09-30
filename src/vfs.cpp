#include "vfs.h"
#include "sha256.h"
#include "base64.h"
#include "json.h"

#include <fstream>
#include <sstream>
#include <iostream>

VirtualFileSystem::VirtualFileSystem()
    : vfsName_("default_vfs"), sha256Hash_(""), totalNodesCount_(1) {
    root_ = std::make_shared<VfsNode>("/", VfsNodeType::Directory);
    currentDir_ = root_;
}

size_t VirtualFileSystem::countNodesRecursive(const std::shared_ptr<VfsNode>& node) const {
    if (!node) return 0;
    size_t count = 1;
    for (const auto& pair : node->children) {
        count += countNodesRecursive(pair.second);
    }
    return count;
}

void VirtualFileSystem::recalculateNodesCount() {
    totalNodesCount_ = countNodesRecursive(root_);
}

void VirtualFileSystem::buildNodeFromJson(const JsonValue& jsonVal, 
                                          const std::shared_ptr<VfsNode>& parentNode, 
                                          const std::string& nodeName) {
    if (!jsonVal.isObject()) return;

    std::string typeStr = "dir";
    if (jsonVal.contains("type") && jsonVal["type"].isString()) {
        typeStr = jsonVal["type"].asString();
    }

    if (typeStr == "dir") {
        auto dirNode = std::make_shared<VfsNode>(nodeName, VfsNodeType::Directory);
        dirNode->parent = parentNode.get();
        if (parentNode) {
            parentNode->children[nodeName] = dirNode;
        }

        if (jsonVal.contains("children") && jsonVal["children"].isObject()) {
            const auto& childrenMap = jsonVal["children"].asObject();
            for (const auto& entry : childrenMap) {
                buildNodeFromJson(entry.second, dirNode, entry.first);
            }
        }
    } else {
        auto fileNode = std::make_shared<VfsNode>(nodeName, VfsNodeType::File);
        fileNode->parent = parentNode.get();

        std::string rawContent = "";
        if (jsonVal.contains("content") && jsonVal["content"].isString()) {
            rawContent = jsonVal["content"].asString();
        }

        std::string encoding = "utf-8";
        if (jsonVal.contains("encoding") && jsonVal["encoding"].isString()) {
            encoding = jsonVal["encoding"].asString();
        }

        if (encoding == "base64") {
            fileNode->content = Base64::decode(rawContent);
        } else {
            fileNode->content = rawContent;
        }

        if (parentNode) {
            parentNode->children[nodeName] = fileNode;
        }
    }
}

bool VirtualFileSystem::loadFromJsonFile(const std::string& filePath, std::string& errorMsg) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "Не удалось открыть файл VFS: " + filePath;
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string fileContent = ss.str();

    sha256Hash_ = Sha256::hashString(fileContent);

    JsonValue rootJson;
    try {
        rootJson = JsonValue::parse(fileContent);
    } catch (const std::exception& e) {
        errorMsg = std::string("Ошибка парсинга JSON: ") + e.what();
        return false;
    }

    if (!rootJson.isObject()) {
        errorMsg = "Корневой элемент JSON должен быть объектом";
        return false;
    }

    if (rootJson.contains("name") && rootJson["name"].isString()) {
        vfsName_ = rootJson["name"].asString();
    } else {
        vfsName_ = "vfs";
    }

    root_ = std::make_shared<VfsNode>("/", VfsNodeType::Directory);
    currentDir_ = root_;

    if (rootJson.contains("children") && rootJson["children"].isObject()) {
        const auto& childrenMap = rootJson["children"].asObject();
        for (const auto& entry : childrenMap) {
            buildNodeFromJson(entry.second, root_, entry.first);
        }
    }

    recalculateNodesCount();
    return true;
}

std::string VirtualFileSystem::getCurrentPath() const {
    if (!currentDir_ || currentDir_ == root_) {
        return "/";
    }

    std::vector<std::string> parts;
    VfsNode* curr = currentDir_.get();
    while (curr && curr != root_.get()) {
        parts.push_back(curr->name);
        curr = curr->parent;
    }

    std::string fullPath = "";
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        fullPath += "/" + *it;
    }
    return fullPath;
}

std::shared_ptr<VfsNode> VirtualFileSystem::resolveNode(const std::string& path) const {
    if (path.empty()) return currentDir_;

    std::shared_ptr<VfsNode> curr = (path[0] == '/') ? root_ : currentDir_;

    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (curr->parent) {
                auto parentPtr = (curr->parent == root_.get()) ? root_ : resolveNode(getCurrentPath() + "/..");
                if (parentPtr) {
                    curr = parentPtr;
                }
            }
            continue;
        }

        if (!curr->isDirectory()) {
            return nullptr;
        }

        auto it = curr->children.find(segment);
        if (it == curr->children.end()) {
            return nullptr;
        }
        curr = it->second;
    }

    return curr;
}

bool VirtualFileSystem::changeDirectory(const std::string& path, std::string& errorMsg) {
    if (path.empty() || path == "~") {
        currentDir_ = root_;
        return true;
    }

    auto target = resolveNode(path);
    if (!target) {
        errorMsg = "cd: " + path + ": No such file or directory";
        return false;
    }

    if (!target->isDirectory()) {
        errorMsg = "cd: " + path + ": Not a directory";
        return false;
    }

    currentDir_ = target;
    return true;
}
