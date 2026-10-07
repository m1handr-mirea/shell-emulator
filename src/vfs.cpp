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
        dirNode->parent = parentNode;
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
        fileNode->parent = parentNode;

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
    auto curr = currentDir_;
    while (curr && curr != root_) {
        parts.push_back(curr->name);
        curr = curr->parent.lock();
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
            auto parent = curr->parent.lock();
            if (parent) {
                curr = parent;
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

bool VirtualFileSystem::makeDirectory(const std::string& path, bool createParents, std::string& errorMsg) {
    if (path.empty()) {
        errorMsg = "mkdir: missing operand";
        return false;
    }

    std::vector<std::string> segments;
    std::stringstream ss(path);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (!seg.empty()) {
            segments.push_back(seg);
        }
    }

    if (segments.empty()) {
        if (path[0] == '/') {
            if (createParents) return true;
            errorMsg = "mkdir: cannot create directory '/': File exists";
            return false;
        }
        errorMsg = "mkdir: missing operand";
        return false;
    }

    std::shared_ptr<VfsNode> curr = (path[0] == '/') ? root_ : currentDir_;

    for (size_t i = 0; i < segments.size(); ++i) {
        const std::string& name = segments[i];
        bool isLast = (i == segments.size() - 1);

        if (name == ".") {
            continue;
        }
        if (name == "..") {
            auto parent = curr->parent.lock();
            if (parent) curr = parent;
            continue;
        }

        if (!curr->isDirectory()) {
            errorMsg = "mkdir: cannot create directory '" + path + "': Not a directory";
            return false;
        }

        auto it = curr->children.find(name);
        if (it != curr->children.end()) {
            if (isLast) {
                if (createParents && it->second->isDirectory()) return true;
                errorMsg = "mkdir: cannot create directory '" + path + "': File exists";
                return false;
            } else {
                if (!it->second->isDirectory()) {
                    errorMsg = "mkdir: cannot create directory '" + path + "': Not a directory";
                    return false;
                }
                curr = it->second;
            }
        } else {
            if (!isLast && !createParents) {
                errorMsg = "mkdir: cannot create directory '" + path + "': No such file or directory";
                return false;
            }

            auto newDir = std::make_shared<VfsNode>(name, VfsNodeType::Directory);
            newDir->parent = curr;
            curr->children[name] = newDir;
            curr = newDir;
        }
    }

    recalculateNodesCount();
    return true;
}

bool VirtualFileSystem::moveNode(const std::string& srcPath, const std::string& destPath, std::string& errorMsg) {
    if (srcPath.empty() || destPath.empty()) {
        errorMsg = "mv: missing file operand";
        return false;
    }

    auto srcNode = resolveNode(srcPath);
    if (!srcNode) {
        errorMsg = "mv: cannot stat '" + srcPath + "': No such file or directory";
        return false;
    }

    if (srcNode == root_) {
        errorMsg = "mv: cannot move root directory";
        return false;
    }

    auto srcOldParent = srcNode->parent.lock();
    if (!srcOldParent) {
        errorMsg = "mv: source node has no parent";
        return false;
    }

    auto destNode = resolveNode(destPath);
    std::shared_ptr<VfsNode> targetParent = nullptr;
    std::string targetName = "";

    if (destNode) {
        if (destNode->isDirectory()) {
            targetParent = destNode;
            targetName = srcNode->name;
        } else {
            if (srcNode->isDirectory()) {
                errorMsg = "mv: cannot overwrite non-directory '" + destPath + "' with directory '" + srcPath + "'";
                return false;
            }
            targetParent = destNode->parent.lock();
            targetName = destNode->name;
        }
    } else {
        size_t lastSlash = destPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string parentPath = destPath.substr(0, lastSlash);
            if (parentPath.empty()) parentPath = "/";
            targetName = destPath.substr(lastSlash + 1);
            targetParent = resolveNode(parentPath);
        } else {
            targetParent = currentDir_;
            targetName = destPath;
        }

        if (!targetParent || !targetParent->isDirectory()) {
            errorMsg = "mv: cannot move to '" + destPath + "': No such file or directory";
            return false;
        }
    }

    if (!targetParent) {
        errorMsg = "mv: invalid destination";
        return false;
    }

    if (srcNode->isDirectory()) {
        auto check = targetParent;
        while (check) {
            if (check == srcNode) {
                errorMsg = "mv: cannot move '" + srcPath + "' to a subdirectory of itself";
                return false;
            }
            check = check->parent.lock();
        }
    }

    srcOldParent->children.erase(srcNode->name);

    srcNode->name = targetName;
    srcNode->parent = targetParent;

    targetParent->children[targetName] = srcNode;

    recalculateNodesCount();
    return true;
}
