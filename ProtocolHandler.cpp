#include "ProtocolHandler.h"
#include "DBManager.h"
#include "FileUtils.h"        
#include <iostream>
#include <windows.h>  // toUTF8 �Լ��� �ʿ�
using namespace std;

string toUTF8_safely(const string& cp949Str) {
    // CP949 �� UTF-16
    int wlen = MultiByteToWideChar(949, 0, cp949Str.data(), (int)cp949Str.size(), nullptr, 0);
    if (wlen == 0) return "���ڵ� ����";

    wstring wide(wlen, 0);
    MultiByteToWideChar(949, 0, cp949Str.data(), (int)cp949Str.size(), &wide[0], wlen);

    // UTF-16 �� UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, nullptr, 0, nullptr, nullptr);
    string utf8(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, &utf8[0], ulen, nullptr, nullptr);

    return utf8;
}

// [ȸ������ handler]
nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
    // ����� ��� (CP949 ���� �ֿܼ� ���)
    cout << u8"[REGISTER_PARENTS] ��û:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "REGISTER_PARENTS";

    //���� �� �� ���� �Ҵ�
    int uid = -1;
    string error_msg;
    //���� ����� �˾ƾ��ϴ� ������ �Ľ��ؼ� �ְ� ���� ���� ����� ����~
    //Ŭ�� �� �� ������ �Ľ�
    bool result = db.registerParents(
        json.value("ID", ""),
        json.value("PW", ""),
        json.value("NICKNAME", ""),
        json.value("PHONE_NUMBER", ""),
        uid, error_msg
    );

    if (result) {

        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"���� �Ϸ�";  // UTF-8 Ŭ���̾�Ʈ��
        response["PARENTS_UID"] = uid;

        cout << u8"�� [REGISTER_PARENTS] ȸ������ ���� - UID: " << uid << endl;
        cout << u8"[REGISTER_PARENTS] ID: " << json.value("ID", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PW: " << json.value("PW", "null") << endl;
        cout << u8"[REGISTER_PARENTS] NICKNAME: " << json.value("NICKNAME", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PHONE_NUMBER: " << json.value("PHONE_NUMBER", "null") << endl;

    }
    else {
        cout << u8"�� [REGISTER_PARENTS] ȸ������ ����" << endl;

        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}

// [�α��� handler]
nlohmann::json ProtocolHandler::handle_Login(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "LOGIN";

    // [1] ���� JSON �α� ���
    cout << u8"[LOGIN] ��û:\n" << json.dump(2) << endl;

    // [2] ���� ����
    int parents_uid = -1;
    string nickname;
    vector<std_child_info> children;
    string error_msg;

    // [3] DB �۾�
    string id = json.value("ID", "");
    string pw = json.value("PW", "");
    bool result = db.login(id, pw, parents_uid, nickname, children, error_msg);

    if (result) {

        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"�α��� ����";
        response["PARENTS_UID"] = parents_uid;
        response["NICKNAME"] = nickname;

        nlohmann::json children_array = nlohmann::json::array();
        for (const auto& child : children) {
            cout << u8"[LOGIN] �ڳ� ����: UID=" << child.child_uid << u8", �̸�=" << child.name << endl;
            children_array.push_back({
                {"CHILDUID", child.child_uid},
                {"NAME", child.name},
                {"GENDER", child.gender},
                {"BIRTH", child.birth},
                {"ICON_COLOR", child.icon_color}
                });
        }

        cout << u8"[LOGIN] �� �ڳ� ��: " << children.size() << endl;
        response["CHILDREN"] = children_array;
        cout << u8"�� [LOGIN] �α��� ���� - UID: " << parents_uid << u8", �г���: " << nickname << endl;
    }

    else {
        cout << u8"�� [LOGIN] �α��� ����: " << toUTF8_safely(error_msg) << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}

//[�ڳ� ��� handler]
nlohmann::json ProtocolHandler::handle_RegisterChild(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "REGISTER_CHILD";

    // [1] ���� JSON �α� ���
    cout << u8"[REGISTER_CHILD] ��û:\n" << json.dump(2) << endl;
    // ���� �� ��
    int out_child_uid = -1; 
    // �Ľ��� ���� ���⼭ ���� Ŭ�� �� ��
    int parents_uid = json.value("PARENTS_UID", -1);
    string babyname = json.value("NAME", "");
    string birthdate = json.value("BIRTHDATE", "");
    string gender = json.value("GENDER","");
    string icon_color = json.value("ICON_COLOR", "");
    string error_msg;

    bool result = db.registerChild(parents_uid, babyname, birthdate, gender, icon_color, out_child_uid, error_msg);

    if (result) {
        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"�ڳ� ��� ����";
        response["CHILD_UID"] = out_child_uid;

        cout << u8"�� [REGISTER_CHILD] �ڳ� ��� ���� - UID: " << out_child_uid << endl;

        // �θ� ID �߰� ��ȸ
        string parentId;
        if (db.getParentIdByUID(parents_uid, parentId)) {
            if (CreateChildDirectory(parentId, parents_uid, babyname, out_child_uid)) {
                cout << u8"�� [REGISTER_CHILD] �ڳ� ���� ���� �Ϸ�" << endl;
            }
            else {
                cerr << u8"�� [REGISTER_CHILD] �ڳ� ���� ���� ����!" << endl;
            }
        }
        else {
            cerr << u8"�� [REGISTER_CHILD] �θ� ID ��ȸ ���� - �ڳ� ���� ���� �Ұ�" << endl;
        }
    }
    else {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"[REGISTER_CHILD] �ڳ� ��� ����: "
             << toUTF8_safely(error_msg) << endl;
    }
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] ��û:\n" << json.dump(2) << endl;

    string title;
    int weather;
    string create_at;
    vector <string> emotions;
    string error_msg;

    int child_uid = json.value("CHILD_UID", -1);
    if (child_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "CHILD_UID ������";
        return response;
    }

    bool success = db.getLatestDiary(child_uid, title, weather, create_at, emotions, error_msg);
    if (!success) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // ���� �� ���� ����
    response["RESP"] = "SUCCESS";
    response["TITLE"] = title;
    response["WEATHER"] = weather;
    response["CREATE_AT"] = create_at;

    nlohmann::json emotion_array = nlohmann::json::array();
    for (const auto& emo : emotions) {
        emotion_array.push_back({ {"EMOTION", emo} });
    }
    response["EMOTIONS"] = emotion_array;
    return response;
}

