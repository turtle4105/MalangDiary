#pragma once
#include <nlohmann/json.hpp>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

class DBManager;

class ProtocolHandler {
public:
    // 회원가입
    static nlohmann::json handle_RegisterParents(const nlohmann::json& json, DBManager& db);
    // 로그인
    static nlohmann::json handle_Login(const nlohmann::json& json, DBManager& db);
    // 아이등록
    static nlohmann::json handle_RegisterChild(const nlohmann::json& json, DBManager& db);
    // 오늘 일기 조회
    static nlohmann::json handle_LatestDiary(const nlohmann::json& json, DBManager& db);
    // 목소리 세팅 파일
    static nlohmann::json handle_SettingVoice(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);
    // 특정 일기 조회
    static nlohmann::json handle_SendDiaryDetail(const nlohmann::json& json, DBManager& db, SOCKET clientSocket);
    // 일기 삭제
    static nlohmann::json handle_DiaryDel(const nlohmann::json& json, DBManager& db);
    // 일기 좋아요
    static nlohmann::json handle_UpdateLike(const nlohmann::json& json, DBManager& db);
    // 일기 수정
    static nlohmann::json handle_ModifyDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);

    static nlohmann::json handle_Calendar(const nlohmann::json& json, DBManager& db);

    static nlohmann::json handle_GenDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);

    static nlohmann::json handle_GenDiary_SaveVoice(const std::string& json_str, const std::vector<char>& payload, DBManager& db);
};


