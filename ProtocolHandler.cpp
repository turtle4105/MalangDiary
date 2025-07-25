#include "ProtocolHandler.h"
#include "DBManager.h"
#include "TcpServer.h"
#include "FileUtils.h"   
#include "DiaryGenerator.h"
#include <sstream>   // << �̰� �ݵ��!
#include <iomanip>   // std::put_time ����� ���� �ʿ�
#include <ctime>
#include <iostream>
#include <fstream>
#include <windows.h>  // toUTF8 �Լ��� �ʿ�
using namespace std;

std::string toAbsolutePath(const std::string& relativePath) {
    char buffer[MAX_PATH];
    DWORD len = GetFullPathNameA(relativePath.c_str(), MAX_PATH, buffer, NULL);
    if (len == 0) {
        return relativePath; // fallback
    }
    return std::string(buffer);
}

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
        response["CHILDREN"] = children_array;
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
    cout << response.dump(2) << endl;
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] ��û:\n" << json.dump(2) << endl;

    // [1] ���� ���� ����
    // - �ϱ�
    string title;
    string weather;
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

// [Ư�� �ϱ� ��ȸ handler]
void ProtocolHandler::handle_SendDiaryDetail(const nlohmann::json& json, DBManager& db, SOCKET clientSocket) {
    nlohmann::json response;
    response["PROTOCOL"] = "SEND_DIARY_DETAIL";
    cout << u8"[SEND_DIARY_DETAIL] ��û:\n" << json.dump(2) << endl;

    // [1] �ʵ� ����
    int diary_uid = json.value("DIARY_UID", -1);
    string title;
    string text;
    string weather;
    int is_liked = 0;
    string photo_path;
    string voice_path;
    string create_at;
    vector<string> emotions;
    string out_error_msg;

    // [2] �ʼ� �Ķ���� Ȯ��
    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"DIARY_UID ������";
        TcpServer::sendJsonResponse(clientSocket, response.dump());
        return;
    }

    // [3] DB���� �ϱ� �� ��ȸ (voice_path ���Եǵ��� getDiaryDetailByUID ���� �ʿ�)
    if (!db.getDiaryDetailByUID(diary_uid, title, text, weather, is_liked, photo_path, voice_path, create_at, emotions, out_error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = out_error_msg;
        TcpServer::sendJsonResponse(clientSocket, response.dump());
        return;
    }

    // [4] ���� �迭 ����
    nlohmann::json emotion_array = nlohmann::json::array();
    for (const auto& emo : emotions) {
        emotion_array.push_back({ {"EMOTION", emo} });
    }

    // [5] JSON ���� ������ ����
    response["RESP"] = "SUCCESS";
    response["DIARY_UID"] = diary_uid;
    response["TITLE"] = title;
    response["TEXT"] = text;
    response["WEATHER"] = weather;
    response["IS_LIKED"] = is_liked;
    response["PHOTO_PATH"] = photo_path;
    response["CREATE_AT"] = create_at;
    response["EMOTIONS"] = emotion_array;
    response["MESSAGE"] = u8"��ȸ ����";

    // [6] ���� ���� ��� ���� �� �ε�
    std::vector<char> voice_payload;

    if (!voice_path.empty()) {
        // [1] �տ� '/' ���� (��� ��η� �����)
        std::string cleanVoicePath = (voice_path[0] == '/') ? voice_path.substr(1) : voice_path;

        // [2] ���� ��� ����
        std::string abs_voice_path =  cleanVoicePath;
        std::replace(abs_voice_path.begin(), abs_voice_path.end(), '/', '\\');

        // [3] ���� �ε� �õ�
        std::ifstream voice_file(abs_voice_path, std::ios::binary);
        if (voice_file) {
            voice_payload.assign(std::istreambuf_iterator<char>(voice_file), std::istreambuf_iterator<char>());
            cout << u8"�� [SEND_DIARY_DETAIL] ���� ���� �ε� �Ϸ� (" << voice_payload.size() << " bytes)" << endl;
        }
        else {
            cerr << u8"[����] ���� ���� ���� ����: " << abs_voice_path << endl;
        }
    }
    else {
        cout << u8"[INFO] voice_path ��� ����. ���� ���� ���� ������." << endl;
    }

    // [7] ��Ÿ JSON + ���� payload ����
    if (voice_payload.empty()) {
        TcpServer::sendJsonResponse(clientSocket, response.dump());
    }
    else {
        TcpServer::sendPacket(clientSocket, response.dump(), voice_payload);
    }

    // [8] ������ ������ ��� �� ��° ����
    if (!photo_path.empty()) {
        std::string cleanPath = photo_path;
        if (!cleanPath.empty() && cleanPath[0] == '/')
            cleanPath = cleanPath.substr(1);

        std::string abs_path = "user_data/" + cleanPath;
        std::replace(abs_path.begin(), abs_path.end(), '/', '\\');

        std::ifstream file(abs_path, std::ios::binary);
        if (file) {
            std::vector<char> payload((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            nlohmann::json img_json;
            img_json["PROTOCOL"] = "SEND_DIARY_IMG";
            img_json["RESP"] = "SUCCESS";
            img_json["DIARY_UID"] = diary_uid;
            img_json["MESSAGE"] = u8"�̹��� ���� ����";

            TcpServer::sendPacket(clientSocket, img_json.dump(), payload);
            cout << u8"�� [SEND_DIARY_IMG] ���� ���� �Ϸ� (" << payload.size() << " bytes)\n";
        }
        else {
            cerr << u8"[����] ���� ���� ���� ����: " << abs_path << endl;
        }
    }

    // [9] ������ ���
    cout << response.dump(2) << endl;
}
// [�ϱ� ���� handler]
nlohmann::json ProtocolHandler::handle_DiaryDel(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    cout << u8"[DIARY_DEL] ��û:\n" << json.dump(2) << endl;

    response["PROTOCOL"] = "DIARY_DEL";
    string error_msg;

    int diary_uid = json.value("DIARY_UID", -1);
    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "DIARY_UID ������";
        return response;
    }
    if (!db.deleteDiaryByUid(diary_uid, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"�� [DIARY_DEL] �ϱ� ���� ����" << endl;

        return response;
    }

    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"�ϱ� ���� �Ϸ�";
    cout << u8"�� [DIARY_DEL] �ϱ� ���� �Ϸ�" << endl;

    return response;

}

// [�ϱ� ���ƿ� ���� handler]
nlohmann::json ProtocolHandler::handle_UpdateLike(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    cout << u8"[UPDATE_DIARY_LIKE] ��û:\n" << json.dump(2) << endl;

    response["PROTOCOL"] = "UPDATE_DIARY_LIKE";
    string error_msg;

    int diary_uid = json.value("DIARY_UID", -1);
    int is_liked = json.value("IS_LIKED", -1);

    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "DIARY_UID ������";
        return response;
    }

    if (!db.Update_DiaryLiked(diary_uid, is_liked, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"�� [UPDATE_DIARY_LIKE] ���ƿ� ���� ���� ����" << endl;

        return response;
    }

    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"���ƿ� ���� ���� �Ϸ�";
    cout << u8"�� [DIARY_DEL] ���ƿ� ���� ���� �Ϸ�" << endl;

    return response;

}

// [�ϱ� ���� handler]
nlohmann::json ProtocolHandler::handle_ModifyDiary(const nlohmann::json& json, const vector<char>& payload, DBManager& db) {
    cout << u8"[MODIFY_DIARY] ��û:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "MODIFY_DIARY";

    // [1] �ʵ� �Ľ�
    int child_uid = json.value("CHILD_UID", -1);
    int diary_uid = json.value("DIARY_UID", -1);
    std::string title = json.value("TITLE", "");
    std::string text = json.value("TEXT", "");
    int is_liked = json.value("IS_LIKED", 0);

    std::string weatherStr = json.value("WEATHER", "");

    std::string photo_filename = json.value("PHOTO_FILENAME", "");
    std::string create_at = json.value("CREATE_AT", "");

    // [2] ���� ����
    std::vector<std::string> emotions;
    for (const auto& e : json.value("EMOTIONS", nlohmann::json::array())) {
        emotions.push_back(e.value("EMOTION", ""));
    }

    // [3] �θ� ID ��ȸ
    int parent_uid;
    std::string parent_id;
    if (!db.getParentsUidByChild(child_uid, parent_uid) ||
        !db.getParentIdByUID(parent_uid, parent_id)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�θ� ���� ��ȸ ����";
        return response;
    }

    // [4] ��� ����
    std::string abs_path = GetChildFolderPath(parent_id, parent_uid, child_uid) + "\\image\\" + photo_filename;
    std::string photo_path = "/" + parent_id + "_" + std::to_string(parent_uid) + "/" +
        std::to_string(child_uid) + "/image/" + photo_filename;
    std::replace(abs_path.begin(), abs_path.end(), '/', '\\');

    // [5] ���� ���� (payload�� ���� ��츸)
    if (!payload.empty()) {
        cout << u8"�� [���� ���� ���] " << abs_path << endl;
        if (!SaveBinaryFile(abs_path, payload)) {
            response["RESP"] = "FAIL";
            response["MESSAGE"] = u8"���� ���� ����";
            return response;
        }
    }

    else {
        cout << u8"�� [���� ���� �ǳʶ�] payload ����" << endl;
    }

    // [6] DB ����
    std::string error_msg;
    bool result = db.Modify_diary(diary_uid, title, text, weatherStr, is_liked, create_at, emotions, photo_path, error_msg);

    if (!result) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // [7] ���� ����
    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"�ϱ� ���� ����";

    cout << u8"�� [MESSAGE] �ϱ� ���� �ݿ� �Ϸ�" << endl;
    cout << response.dump(2);
    return response;
}
// [�޷� ��ȸ handler]
nlohmann::json ProtocolHandler::handle_Calendar(const nlohmann::json& json, DBManager& db) {
    cout << u8"[handle_Calendar] ��û:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "SEND_CALENDAR_REQ";

    // [1] �ʵ� �Ľ�
    int child_uid = json.value("CHILD_UID", -1);
    int year = json.value("YEAR", -1);
    int month = json.value("MONTH", -1);

    if (child_uid == -1 || year == -1 || month == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID �Ǵ� ��¥ ���� ������";
        return response;
    }

    // [2] DB ȣ��
    std::vector<CalendarData> calendar;
    std::string error_msg;

    if (!db.getCalendarData(child_uid, year, month, calendar, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }
    // [3] ���� ������ ����
    nlohmann::json data_array = nlohmann::json::array();
    for (const auto& entry : calendar) {
        data_array.push_back({
            {"DIARY_UID", entry.diary_uid},
            {"DATE", entry.date},  // ������ ��(day)
            {"IS_WRITED", entry.is_writed},
            {"IS_LIKED", entry.is_liked}
            });
    }
    response["RESP"] = "SUCCESS";
    response["DATA"] = data_array;
    cout << u8"�� [SEND_CALENDAR_REQ] �޷� ��ȸ �Ϸ�" << endl;

    return response;
}

nlohmann::json ProtocolHandler::handle_GenDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db) {
    cout << u8"[handle_GenDiary] ��û:\n" << json.dump(2) << endl;
    nlohmann::json response;
    response["PROTOCOL"] = "GEN_DIARY_RESULT";

    // ��û �ʵ� �Ľ� ~ DB��
    int child_uid = json.value("CHILD_UID", -1);
    string child_name = json.value("NAME", "");
    string out_error_msg;
    std::time_t t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    int out_diary_uid;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    string today_str = oss.str();

    response["DATE"] = today_str;

    // ���� ���� �ܰ�
    nlohmann::json saveResp = handle_GenDiary_SaveVoice(json, payload, db);
    if (saveResp.value("RESP", "") != "SUCCESS") {
        return saveResp; //���� �޽��� �״�� Ŭ������ �ַ���
    }
    cout << u8"[handle_GenDiary-1] ���� ���� ����. VOICE_PATH: " << saveResp.value("VOICE_PATH", "") << endl;
    // ��û �ʵ� �Ľ� ~ ���̽㲨
    string audio_path = saveResp.value("VOICE_PATH", "");
    string embedding_data;

    bool getVoiceVector_result = db.getVoiceVector(child_uid, embedding_data, out_error_msg);

    cout << "[handle_GenDiary] getVoiceVector_result: " << getVoiceVector_result << endl;

    cout << u8"[handle_GenDiary-2] DB���� �Ӻ��� ���� �ҷ����� ��..." << endl;
    if (!getVoiceVector_result) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�Ӻ��� DB �ε� ����: " + out_error_msg;
        return response;
    }

    string embedding_only_str;
    if (!DiaryGenerator::extract_embedding_array(embedding_data, embedding_only_str, out_error_msg)) {  // ��ȣ ����, �����ݷ� �߰�
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�Ӻ��� ���ڿ� ��ȯ ����: " + out_error_msg;
        return response; // return �����Ǿ���
    }

    cout << u8"[handle_GenDiary-3] �Ӻ��� �ҷ����� ����, ����: " << embedding_data.size() << " bytes" << endl;

    if (child_uid == -1 || child_name.empty() || audio_path.empty() || embedding_data.empty()) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�ʼ� �ʵ� ���� (CHILD_UID, CHILD_NAME, AUDIO_PATH, EMBEDDING)";
        return response;
    }
    cout << u8"[handle_GenDiary-4] �Ķ���� ��ȿ�� ���" << endl;

    // Python ������ ����
    cout << u8"[handle_GenDiary-5] Python ���� ȣ�� ����" << endl;

    nlohmann::json diary_data;
    string error_msg;
    if (!DiaryGenerator::sendToPythonDiaryServer(audio_path, embedding_only_str, child_name, diary_data, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = error_msg;
        return response;
    }
    cout << u8"[handle_GenDiary-6] Python ���� ����\n�� title: " << diary_data.value("title", "");
    // ���� ��¥ ���

    // ���� ����Ʈ �Ľ�
    std::vector<std::string> emotion_list;
    for (const auto& e : diary_data["emotion"]) {
        emotion_list.push_back(e);
    }

    bool insert_result = db.insertGeneratedDiary(
        child_uid,
        diary_data.value("title", ""),
        diary_data.value("content", ""),
        diary_data.value("weather", ""),  // weather �ʱⰪ �Ǵ� ���ð����� ����
        audio_path,
        emotion_list,
        today_str,
        out_error_msg,
        out_diary_uid
    );

    if (!insert_result) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = out_error_msg;
        return response;
    }
    // ���� ����
    response["RESP"] = "SUCCESS";
    response["DIARY_UID"] = out_diary_uid;
    response["TITLE"] = diary_data.value("title", "");
    response["TEXT"] = diary_data.value("content", "");
    response["EMOTIONS"] = nlohmann::json::array();
    for (const auto& emo : diary_data["emotion"]) {
        response["EMOTIONS"].push_back({ {"EMOTION", emo} });
    }
    cout << u8"[handle_GenDiary() Done]" << endl;

    return response;
}

nlohmann::json ProtocolHandler::handle_GenDiary_SaveVoice(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GEN_DIARY_RESULT";
    cout << u8"[handle_GenDiary_SaveVoice] ����" << endl;

    int child_uid = json.value("CHILD_UID", -1);
    string child_name = json.value("NAME", "");
    std::string filename;  // ���߿� ��¥ ������� ä�� ����

    cout << u8"[handle_GenDiary_SaveVoice] �ʵ� �Ľ� �Ϸ�" << endl;

    if (child_uid < 0 || child_name.empty()) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID �Ǵ� NAME ����";
        return response;
    }
    
    // 2) �θ� ID �� parentId ��ȸ
    int parents_uid;
    std::string parentId;
    if (!db.getParentsUidByChild(child_uid, parents_uid) ||
        !db.getParentIdByUID(parents_uid, parentId))
    {   
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"�θ� ���� ��ȸ ����";
        return response;
    }
    cout << u8"[handle_GenDiary_SaveVoice -1] �θ� ���� ��ȸ" << endl;

    // 3) child voice ���丮 ���

    cout << u8"parentsuid"<<parents_uid<<"parentsid" << parentId << "child_uid" << child_uid << endl;

    std::string childPath = GetChildFolderPath(parentId, parents_uid, child_uid);
    std::string voiceDir = childPath + "\\voice";

    // 4) ���� ��¥ ��� ���ϸ� ����
    std::time_t t = std::time(nullptr);
    std::tm    tm;
    localtime_s(&tm, &t);
    {
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d") << ".wav";
        filename = oss.str();
    }
    // 5) ��ü ���� ���
    std::string fullPath = voiceDir + "\\" + filename;

    // 6) ���� ����
    if (!SaveBinaryFile(fullPath, payload)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"���� ���� ���� ����";
        return response;
    }
    cout << u8"[handle_GenDiary_SaveVoice-2] ���� ���� ����" << endl;


    // **���⼭ ���� ����**
    response["RESP"] = "SUCCESS";
    response["VOICE_PATH"] = fullPath;
    cout << u8"�� [VOICE_PATH] �������� ���� �Ϸ�!! >0<" << endl;
    cout << "handle_GenDiary_SaveVoice() Done" << endl;

    return response;
}

