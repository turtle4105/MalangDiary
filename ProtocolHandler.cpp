#include "ProtocolHandler.h"
#include "DBManager.h"
#include "FileUtils.h"        
#include <iostream>
#include <fstream>
#include <windows.h>  // toUTF8 �Լ��� �ʿ�
using namespace std;
std::string toAbsolutePath(const std::string& relativePath);

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
    vector<std_child_info> children;

    // �Ľ��� ���� ���⼭ ���� Ŭ�� �� ��
    int parents_uid = json.value("PARENTS_UID", -1);
    string babyname = json.value("NAME", "");
    string birthdate = json.value("BIRTHDATE", "");
    string gender = json.value("GENDER","");
    string icon_color = json.value("ICON_COLOR", "");
    string error_msg;

    bool result = db.registerChild(parents_uid, babyname, birthdate, gender, children, out_child_uid, icon_color, error_msg);

    if (result) {
        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"�ڳ� ��� ����";
        response["NEW_CHILD"] = out_child_uid; 

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

        cout << u8"�� [REGISTER_CHILD] �ڳ� ��� ���� - UID: " << out_child_uid << endl;

        // �θ� ID �߰� ��ȸ
        string parentId;
        if (db.getParentIdByUID(parents_uid, parentId)) {
            if (CreateChildDirectory(parentId, parents_uid, out_child_uid)) {
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
        cout << u8"�� [REGISTER_CHILD] �ڳ� ��� ����: "
             << toUTF8_safely(error_msg) << endl;
    }
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] ��û:\n" << json.dump(2) << endl;

    // [1] ���� ���� ����
    // - �ϱ�
    string title;
    int weather;
    int diary_uid;
    string create_at;
    vector<string> emotions;
    string error_msg;

    // - ����
    std::string photo_path;
    size_t photo_size = 0;
    bool has_photo = false;


    // [2] ��û���κ��� �ڳ� UID ����
    int child_uid = json.value("CHILD_UID", -1);
    if (child_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID ������";
        return response;
    }

    // [3] �ֽ� �ϱ� ��ȸ (DB)
    bool success = db.getLatestDiary(child_uid, diary_uid, photo_path, title, weather, create_at, emotions, error_msg);

    // [4] ��ȸ ���� �� ���� ��ȯ
    if (!success) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // [5] �θ� ���� ��ȸ (���� ��� ������ ����)
    int parent_uid;
    std::string parent_id;
    if (db.getParentsUidByChild(child_uid, parent_uid) &&
        db.getParentIdByUID(parent_uid, parent_id)) {

        // [6] �ϱ� ���� ��¥ ��� ���� ���� Ž��

        if (!photo_path.empty()) {
            std::string cleanPhotoPath = photo_path;
            if (!cleanPhotoPath.empty() && cleanPhotoPath[0] == '/')
                cleanPhotoPath = cleanPhotoPath.substr(1);
            // [7] ���� ��� ����
            std::string abs_path = "user_data/" + cleanPhotoPath;
            std::replace(abs_path.begin(), abs_path.end(), '/', '\\');
            // [8] ������
            std::ifstream file(abs_path, std::ios::binary | std::ios::ate);
            if (file) {
                photo_size = static_cast<size_t>(file.tellg());
                has_photo = true;
            }
        }

        // [9] ���� ���� ���� JSON�� ����
        response["HAS_PHOTO"] = has_photo;              // ���� ���� ����
        response["PHOTO_PATH"] = photo_path;            // ��� ���
        response["PHOTO_SIZE"] = photo_size;            // ���� ũ�� (Ŭ���̾�Ʈ�� �̸�ŭ ������)

        // [10] ���� �迭 ����
        nlohmann::json emotion_array = nlohmann::json::array();
        for (const auto& emo : emotions) {
            emotion_array.push_back({ {"EMOTION", emo} });
        }

        // [11] ���� ���� ����
        response["RESP"] = "SUCCESS";
        response["TITLE"] = title;
        response["WEATHER"] = weather;
        response["CREATE_AT"] = create_at;
        response["DIARY_UID"] = diary_uid;
        response["EMOTIONS"] = emotion_array;

        cout << u8"�� [GET_LATEST_DIARY] ������ �ϱ� ���� �Ϸ� ";
        cout << response.dump(2);
        return response;
    }
}

nlohmann::json ProtocolHandler::handle_SettingVoice(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "SETTING_VOICE";
    cout << u8"[SETTING_VOICE] ��û:\n" << json.dump(2) << endl;

    // [1] �ʵ� ����
    int child_uid = json.value("CHILD_UID", -1);
    string filename = json.value("FILENAME", "");

    // [2] �θ� ���� ��ȸ
    int parent_uid = -1;
    string parent_id;
    if (!db.getParentsUidByChild(child_uid, parent_uid) ||
        !db.getParentIdByUID(parent_uid, parent_id)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�θ� ���� ��ȸ ����";
        cout << u8"��[DIRECTORY] �θ� ���� ��ȸ ����" << endl;
        return response;
    }

    // [3] �ڳ� ���� ���
    //string child_dir = GetChildFolderPath(parent_id, parent_uid, child_uid);

    // [4] ��ü ��� ����
    string full_path = GetChildFolderPath(parent_id, parent_uid, child_uid) + "\\" + filename;
    std::string abs_path = toAbsolutePath(full_path);
    std::replace(abs_path.begin(), abs_path.end(), '\\', '/');

    // [5] ������ .wav ���� ����
    if (!SaveBinaryFile(full_path, payload)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"���� ���� ����";
        cout << u8"��[SaveBinaryFile] ���� ����" << endl;
        return response;
    }
    cout << u8"��[SaveBinaryFile] ���� ���� ����" << endl;

    // [6] ���� ����
    std::string command = "venv\\Scripts\\python.exe embed_child_voice.py \"" + full_path + "\" " + std::to_string(child_uid);
    FILE* pipe = _popen(command.c_str(), "r");
    
    cout << u8"[DEBUG] full_path: " << full_path << endl;
    cout << u8"[DEBUG] abs_path : " << abs_path << endl;
    cout << u8"[DEBUG] ���� ���: " << command << endl;

    if (!pipe) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�Ӻ��� ���� ����";
        cout << u8"��[EMBEDDING] ���� ����" << endl;
        return response;
    }

    char buffer[4096];
    std::string json_output;

    // ���� ���̽� ��� �޾ƿ���
    while (fgets(buffer, sizeof(buffer), pipe)) {
        json_output += buffer;
    }
    _pclose(pipe);

    // ���� JSON ��ϸ� ����
    std::size_t start = json_output.find('{');
    std::size_t end = json_output.rfind('}');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        json_output = json_output.substr(start, end - start + 1);
    }
    else {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"JSON �Ľ� ����: ����� �ùٸ� JSON�� �ƴ�";
        return response;
    }
    // �״�� ���� (�Ľ� �� ��)
    string error_msg;
    if (!db.setVoiceVectorRaw(child_uid, json_output, error_msg)) {
        cout << u8"[ERROR] DB ���� ����: " << error_msg << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"DB ���� ����: " + toUTF8_safely(error_msg);
        cout << u8"��[setVoiceVectorRaw] DB ���� ����" << endl;

        return response;
    }
    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"�Ӻ��� �� ���� �Ϸ�";
    cout << u8"��[EMBEDDING] �Ӻ��� �� ���� �Ϸ�" << endl;
    return response;
}

std::string toAbsolutePath(const std::string& relativePath) {
    char buffer[MAX_PATH];
    DWORD len = GetFullPathNameA(relativePath.c_str(), MAX_PATH, buffer, NULL);
    if (len == 0) {
        return relativePath; // fallback
    }
    return std::string(buffer);
}

