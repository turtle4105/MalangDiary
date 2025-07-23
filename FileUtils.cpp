#include "FileUtils.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#define BASE_DIR "user_data"

bool IsDirectoryExists(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), NULL)) {
        std::cout << u8"[���� ������] " << path << std::endl;
        return true;
    }
    else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) return true;
        std::cerr << u8"[����] ���� ���� ����: " << path << u8" (�ڵ�: " << err << ")\n";
        return false;
    }
}

std::string GetUserFolderPath(const std::string& parentId, int uid) {
    return std::string(BASE_DIR) + "\\" + parentId + "_" + std::to_string(uid);
}

bool CreateUserDirectory(const std::string& parentId, const int& uid) {
    if (!IsDirectoryExists(BASE_DIR)) return false;
    return IsDirectoryExists(GetUserFolderPath(parentId, uid));
}

std::string GetChildFolderPath(const std::string& parentId, int parentUid, int childUid) {
    return GetUserFolderPath(parentId, parentUid) + "\\" + std::to_string(childUid);
}

bool CreateChildDirectory(const std::string& parentId, int parentUid, int childUid) {
    return IsDirectoryExists(GetChildFolderPath(parentId, parentUid, childUid));
}

//bool EnsureSettingFolder(const std::string& childPath) {
//    return IsDirectoryExists(childPath + "\\setting");
//}
//
//std::string GetSettingFolderPath(const std::string& parentId, int parentUid, int childUid) {
//    return GetChildFolderPath(parentId, parentUid, childUid) + "\\setting";
//}

bool SaveBinaryFile(const std::string& path, const std::vector<char>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[����] ���� ���� ����: " << path << std::endl;
        return false;
    }
    file.write(data.data(), data.size());
    file.close();
    return true;
}
