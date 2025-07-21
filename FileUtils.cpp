#include "FileUtils.h"
#include <windows.h>
#include <iostream>

#define BASE_DIR "user_data"

bool IsDirectoryExists(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), NULL)) {
        std::cout << "[폴더 생성됨] " << path << std::endl;
        return true;
    }
    else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) return true;
        std::cerr << "[에러] 폴더 생성 실패: " << path << " (코드: " << err << ")\n";
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

std::string GetChildFolderPath(const std::string& parentId, int parentUid, const std::string& childName, int childUid) {
    return GetUserFolderPath(parentId, parentUid) + "\\" + childName + "_" + std::to_string(childUid);
}

bool CreateChildDirectory(const std::string& parentId, int parentUid, const std::string& childName, int childUid) {
    return IsDirectoryExists(GetChildFolderPath(parentId, parentUid, childName, childUid));
}
