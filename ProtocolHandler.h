#pragma once
#include <nlohmann/json.hpp>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

class DBManager;

class ProtocolHandler {
public:
    // ȸ������
    static nlohmann::json handle_RegisterParents(const nlohmann::json& json, DBManager& db);
    // �α���
    static nlohmann::json handle_Login(const nlohmann::json& json, DBManager& db);
    // ���̵��
    static nlohmann::json handle_RegisterChild(const nlohmann::json& json, DBManager& db);
    // ���� �ϱ� ��ȸ
    static nlohmann::json handle_LatestDiary(const nlohmann::json& json, DBManager& db);
    // ��Ҹ� ���� ����
    static nlohmann::json handle_SettingVoice(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);
    // Ư�� �ϱ� ��ȸ
    static nlohmann::json handle_SendDiaryDetail(const nlohmann::json& json, DBManager& db, SOCKET clientSocket);
    // �ϱ� ����
    static nlohmann::json handle_DiaryDel(const nlohmann::json& json, DBManager& db);
    // �ϱ� ���ƿ�
    static nlohmann::json handle_UpdateLike(const nlohmann::json& json, DBManager& db);
    // �ϱ� ����
    static nlohmann::json handle_ModifyDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);

    static nlohmann::json handle_Calendar(const nlohmann::json& json, DBManager& db);

    static nlohmann::json handle_GenDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db);

    static nlohmann::json handle_GenDiary_SaveVoice(const std::string& json_str, const std::vector<char>& payload, DBManager& db);
};


