//#include "ProtocolHandler.h"
//#include "DBManager.h"
//#include <iostream>
#include "ProtocolHandler.h"
#include "DBManager.h"
#include <iostream>
#include <windows.h>  // toUTF8 �Լ��� �ʿ�
//using namespace std;
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
//
//wstring Utf8ToWstring(const string& str) {
//    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
//    if (len == 0) return L"";
//    wstring result(len, L'\0');
//    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
//    result.pop_back();  // null ����
//    return result;
//}
//
//// =======================================================================
//// [ȸ������ handler]
//// Ŭ���̾�Ʈ�κ��� ���� ȸ������ ��û JSON�� �Ľ��ϰ�,
//// DBManager�� ���� �θ� ������ DB�� ������ ��,
//// ���� ���ο� ���� ���� JSON�� �����Ͽ� ��ȯ�ϴ� �Լ�.
//// =======================================================================
//nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
//
//    // ���� JSON ��ü ����
//    nlohmann::json response;
//    response["PROTOCOL"] = "REGISTER_PARENTS";  // ��û �������� �̸��� ��� 
//
//    // DB ó�� ����� ������ ������
//    int uid = -1;               // �θ� UID (���� ���� �� �Ҵ��)
//    string error_msg;      // ���� �� ������ ���� �޽���
//
//    // DBManager�� ���� ���� ȸ������ �õ�
//    // json���� ID, PW, NICKNAME, PHONE_NUMBER�� �����Ͽ� �ѱ�
//    bool result = db.registerParents(
//        json.value("ID", ""),
//        json.value("PW", ""),
//        json.value("NICKNAME", ""),
//        json.value("PHONE_NUMBER", ""),
//        uid, error_msg
//    );
//
//    cout << "[DEBUG] ���� JSON ����:\n" << json.dump(2) << endl;
//    cout << "ID: " << json.value("ID", "null") << endl;
//    cout << "PW: " << json.value("PW", "null") << endl;
//    cout << "NICKNAME: " << json.value("NICKNAME", "null") << endl;
//    cout << "PHONE_NUMBER: " << json.value("PHONE_NUMBER", "null") << endl;
//
//
//
//    if (result) {
//        // [����] ȸ�������� ���������� �̷������ �� ���� ����
//        response["RESP"] = "SUCCESS";           // ���� ����
//        response["MESSAGE"] = "���� �Ϸ�";      // ����ڿ��� ������ �޽���
//        response["PARENTS_UID"] = uid;          // DB���� ������ �θ� UID
//    }
//    else {
//        // [����] DB ���� ���� �Ǵ� ID �ߺ� ���� ����
//        response["RESP"] = "FAIL";              // ���� ����
//        response["MESSAGE"] = toUTF8_safely(error_msg);
//        // �� ���� ����
//
//        wcout << Utf8ToWstring(response.dump(2)) << endl;
//
//
//
//    }
//
//    // ���� ���� JSON ��ȯ
//    return response;
//}
//
//




// =======================================================================
// [ȸ������ handler]
// Ŭ���̾�Ʈ�κ��� ���� ȸ������ ��û JSON�� �Ľ��ϰ�,
// DBManager�� ���� �θ� ������ DB�� ������ ��,
// ���� ���ο� ���� ���� JSON�� �����Ͽ� ��ȯ�ϴ� �Լ�.
// =======================================================================
nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
    // ����� ��� (CP949 ���� �ֿܼ� ���)
    cout << u8"[REGISTER_PARENTS] ��û:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "REGISTER_PARENTS";

    int uid = -1;
    string error_msg;

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
        cout << u8"�� [REGISTER_PARENTS] ȸ������ ���� :" << endl;

        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}

nlohmann::json ProtocolHandler::handle_Login(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "LOGIN";

    // [1] ���� JSON �α� ���
    cout << u8"[LOGIN] ��û:\n" << json.dump(2) << endl;
    cout << u8"[LOGIN] ID: " << json.value("ID", "null") << endl;
    cout << u8"[LOGIN] PW: " << json.value("PW", "null") << endl;

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
        cout << u8"�� [LOGIN] �α��� ����: " << error_msg << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}
