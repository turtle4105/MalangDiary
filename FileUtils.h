#pragma once
#include <string>
 
bool IsDirectoryExists(const std::string& path);

std::string GetUserFolderPath(const std::string& parentId, int uid);

bool CreateUserDirectory(const std::string& parentId, const int& uid);

std::string GetChildFolderPath(const std::string& parentId, int parentUid,
	                           const std::string& childName, int childUid);

bool CreateChildDirectory(const std::string& parentId, int parentUid,
	                      const std::string& childName, int childUid);
