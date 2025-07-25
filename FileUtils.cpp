#include "FileUtils.h"
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#define BASE_DIR "user_data"

bool IsDirectoryExists(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), NULL)) {
        std::cout << u8"[폴더 생성됨] " << path << std::endl;
        return true;
    }
    else {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) return true;
        std::cerr << u8"[에러] 폴더 생성 실패: " << path << u8" (코드: " << err << ")\n";
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
    std::string childPath = GetChildFolderPath(parentId, parentUid, childUid);

    if (!IsDirectoryExists(childPath)) return false;

    // 하위 디렉토리 생성
    if (!IsDirectoryExists(childPath + "\\image")) return false;
    if (!IsDirectoryExists(childPath + "\\voice")) return false;

    return true;
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
        std::cerr << "[에러] 파일 열기 실패: " << path << std::endl;
        return false;
    }
    file.write(data.data(), data.size());
    file.close();
    return true;
}

std::string FindPhotoByDate(const std::string& parentId, int parentUid, int childUid, const std::string& date) {
    std::string basePath = GetChildFolderPath(parentId, parentUid, childUid) + "\\image";
    std::string pattern = basePath + "\\" + date + "*.jpg";  // 예: 2025-07-23*.jpg

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return "";  // 없음

    std::string foundFile = findFileData.cFileName;
    FindClose(hFind);

    return "/" + parentId + "_" + std::to_string(parentUid) + "/" +
        std::to_string(childUid) + "/image/" + foundFile;  // photo_path용 리턴
}

size_t getFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;

    std::streampos pos = file.tellg();
    return static_cast<size_t>(pos);
}
