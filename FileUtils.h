#pragma once
#include <string>
#include <vector>
 
bool IsDirectoryExists(const std::string& path);

std::string GetUserFolderPath(const std::string& parentId, int uid);

bool CreateUserDirectory(const std::string& parentId, const int& uid);

std::string GetChildFolderPath(const std::string& parentId, int parentUid, int childUid);

bool CreateChildDirectory(const std::string& parentId, int parentUid, int childUid);

//bool EnsureSettingFolder(const std::string& childPath);

//std::string GetSettingFolderPath(const std::string& parentId, int parentUid, int childUid);

bool SaveBinaryFile(const std::string& path, const std::vector<char>& data);

// 날짜로 사진 검색하기
std::string FindPhotoByDate(const std::string& parentId, int parentUid, int childUid, const std::string& date);

size_t getFileSize(const std::string& path);