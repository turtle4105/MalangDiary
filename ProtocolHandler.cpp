#include "ProtocolHandler.h"
#include "DBManager.h"
#include "FileUtils.h"        
#include <iostream>
#include <fstream>
#include <windows.h>  // toUTF8 함수에 필요
using namespace std;
std::string toAbsolutePath(const std::string& relativePath);

string toUTF8_safely(const string& cp949Str) {
    // CP949 → UTF-16
    int wlen = MultiByteToWideChar(949, 0, cp949Str.data(), (int)cp949Str.size(), nullptr, 0);
    if (wlen == 0) return "인코딩 오류";

    wstring wide(wlen, 0);
    MultiByteToWideChar(949, 0, cp949Str.data(), (int)cp949Str.size(), &wide[0], wlen);

    // UTF-16 → UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, nullptr, 0, nullptr, nullptr);
    string utf8(ulen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wlen, &utf8[0], ulen, nullptr, nullptr);

    return utf8;
}

// [회원가입 handler]
nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
    // 디버깅 출력 (CP949 기준 콘솔용 출력)
    cout << u8"[REGISTER_PARENTS] 요청:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "REGISTER_PARENTS";

    //내가 줄 값 공간 할당
    int uid = -1;
    string error_msg;
    //쿼리 실행시 알아야하는 정보는 파싱해서 주고 없는 값은 비워서 전달~
    //클라가 준 값 꺼내서 파싱
    bool result = db.registerParents(
        json.value("ID", ""),
        json.value("PW", ""),
        json.value("NICKNAME", ""),
        json.value("PHONE_NUMBER", ""),
        uid, error_msg
    );

    if (result) {

        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"가입 완료";  // UTF-8 클라이언트용
        response["PARENTS_UID"] = uid;

        cout << u8"→ [REGISTER_PARENTS] 회원가입 성공 - UID: " << uid << endl;
        cout << u8"[REGISTER_PARENTS] ID: " << json.value("ID", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PW: " << json.value("PW", "null") << endl;
        cout << u8"[REGISTER_PARENTS] NICKNAME: " << json.value("NICKNAME", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PHONE_NUMBER: " << json.value("PHONE_NUMBER", "null") << endl;

    }
    else {
        cout << u8"→ [REGISTER_PARENTS] 회원가입 실패" << endl;

        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}

// [로그인 handler]
nlohmann::json ProtocolHandler::handle_Login(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "LOGIN";

    // [1] 받은 JSON 로그 출력
    cout << u8"[LOGIN] 요청:\n" << json.dump(2) << endl;

    // [2] 변수 선언
    int parents_uid = -1;
    string nickname;
    vector<std_child_info> children;
    string error_msg;

    // [3] DB 작업
    string id = json.value("ID", "");
    string pw = json.value("PW", "");
    bool result = db.login(id, pw, parents_uid, nickname, children, error_msg);

    if (result) {

        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"로그인 성공";
        response["PARENTS_UID"] = parents_uid;
        response["NICKNAME"] = nickname;

        nlohmann::json children_array = nlohmann::json::array();
        for (const auto& child : children) {
            cout << u8"[LOGIN] 자녀 정보: UID=" << child.child_uid << u8", 이름=" << child.name << endl;
            children_array.push_back({
                {"CHILDUID", child.child_uid},
                {"NAME", child.name},
                {"GENDER", child.gender},
                {"BIRTH", child.birth},
                {"ICON_COLOR", child.icon_color}
                });
        }

        cout << u8"[LOGIN] 총 자녀 수: " << children.size() << endl;
        response["CHILDREN"] = children_array;
        cout << u8"→ [LOGIN] 로그인 성공 - UID: " << parents_uid << u8", 닉네임: " << nickname << endl;
    }

    else {
        cout << u8"→ [LOGIN] 로그인 실패: " << toUTF8_safely(error_msg) << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }
    return response;
}

//[자녀 등록 handler]
nlohmann::json ProtocolHandler::handle_RegisterChild(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "REGISTER_CHILD";

    // [1] 받은 JSON 로그 출력
    cout << u8"[REGISTER_CHILD] 요청:\n" << json.dump(2) << endl;
    // 내가 줄 값
    int out_child_uid = -1; 
    vector<std_child_info> children;

    // 파싱한 값을 여기서 꺼냄 클라가 준 값
    int parents_uid = json.value("PARENTS_UID", -1);
    string babyname = json.value("NAME", "");
    string birthdate = json.value("BIRTHDATE", "");
    string gender = json.value("GENDER","");
    string icon_color = json.value("ICON_COLOR", "");
    string error_msg;

    bool result = db.registerChild(parents_uid, babyname, birthdate, gender, children, out_child_uid, icon_color, error_msg);

    if (result) {
        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"자녀 등록 성공";
        response["NEW_CHILD"] = out_child_uid; 

        nlohmann::json children_array = nlohmann::json::array();
        for (const auto& child : children) {
            cout << u8"[LOGIN] 자녀 정보: UID=" << child.child_uid << u8", 이름=" << child.name << endl;
            children_array.push_back({
                {"CHILDUID", child.child_uid},
                {"NAME", child.name},
                {"GENDER", child.gender},
                {"BIRTH", child.birth},
                {"ICON_COLOR", child.icon_color}
                });
        }

        cout << u8"→ [REGISTER_CHILD] 자녀 등록 성공 - UID: " << out_child_uid << endl;

        // 부모 ID 추가 조회
        string parentId;
        if (db.getParentIdByUID(parents_uid, parentId)) {
            if (CreateChildDirectory(parentId, parents_uid, out_child_uid)) {
                cout << u8"→ [REGISTER_CHILD] 자녀 폴더 생성 완료" << endl;
            }
            else {
                cerr << u8"→ [REGISTER_CHILD] 자녀 폴더 생성 실패!" << endl;
            }
        }
        else {
            cerr << u8"→ [REGISTER_CHILD] 부모 ID 조회 실패 - 자녀 폴더 생성 불가" << endl;
        }
    }
    else {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"→ [REGISTER_CHILD] 자녀 등록 실패: "
             << toUTF8_safely(error_msg) << endl;
    }
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] 요청:\n" << json.dump(2) << endl;

    // [1] 관련 변수 선언
    // - 일기
    string title;
    int weather;
    int diary_uid;
    string create_at;
    vector<string> emotions;
    string error_msg;

    // - 사진
    std::string photo_path;
    size_t photo_size = 0;
    bool has_photo = false;


    // [2] 요청으로부터 자녀 UID 추출
    int child_uid = json.value("CHILD_UID", -1);
    if (child_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID 누락됨";
        return response;
    }

    // [3] 최신 일기 조회 (DB)
    bool success = db.getLatestDiary(child_uid, diary_uid, photo_path, title, weather, create_at, emotions, error_msg);

    // [4] 조회 실패 시 응답 반환
    if (!success) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // [5] 부모 정보 조회 (폴더 경로 생성을 위함)
    int parent_uid;
    std::string parent_id;
    if (db.getParentsUidByChild(child_uid, parent_uid) &&
        db.getParentIdByUID(parent_uid, parent_id)) {

        // [6] 일기 생성 날짜 기반 사진 파일 탐색

        if (!photo_path.empty()) {
            std::string cleanPhotoPath = photo_path;
            if (!cleanPhotoPath.empty() && cleanPhotoPath[0] == '/')
                cleanPhotoPath = cleanPhotoPath.substr(1);
            // [7] 절대 경로 생성
            std::string abs_path = "user_data/" + cleanPhotoPath;
            std::replace(abs_path.begin(), abs_path.end(), '/', '\\');
            // [8] 사이즈
            std::ifstream file(abs_path, std::ios::binary | std::ios::ate);
            if (file) {
                photo_size = static_cast<size_t>(file.tellg());
                has_photo = true;
            }
        }

        // [9] 사진 관련 정보 JSON에 포함
        response["HAS_PHOTO"] = has_photo;              // 사진 존재 여부
        response["PHOTO_PATH"] = photo_path;            // 상대 경로
        response["PHOTO_SIZE"] = photo_size;            // 파일 크기 (클라이언트는 이만큼 수신함)

        // [10] 감정 배열 구성
        nlohmann::json emotion_array = nlohmann::json::array();
        for (const auto& emo : emotions) {
            emotion_array.push_back({ {"EMOTION", emo} });
        }

        // [11] 최종 응답 구성
        response["RESP"] = "SUCCESS";
        response["TITLE"] = title;
        response["WEATHER"] = weather;
        response["CREATE_AT"] = create_at;
        response["DIARY_UID"] = diary_uid;
        response["EMOTIONS"] = emotion_array;

        cout << u8"→ [GET_LATEST_DIARY] 오늘의 일기 전송 완료 ";
        cout << response.dump(2);
        return response;
    }
}

nlohmann::json ProtocolHandler::handle_SettingVoice(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "SETTING_VOICE";
    cout << u8"[SETTING_VOICE] 요청:\n" << json.dump(2) << endl;

    // [1] 필드 추출
    int child_uid = json.value("CHILD_UID", -1);
    string filename = json.value("FILENAME", "");

    // [2] 부모 정보 조회
    int parent_uid = -1;
    string parent_id;
    if (!db.getParentsUidByChild(child_uid, parent_uid) ||
        !db.getParentIdByUID(parent_uid, parent_id)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"부모 정보 조회 실패";
        cout << u8"→[DIRECTORY] 부모 정보 조회 실패" << endl;
        return response;
    }

    // [3] 자녀 폴더 경로
    //string child_dir = GetChildFolderPath(parent_id, parent_uid, child_uid);

    // [4] 전체 경로 생성
    string full_path = GetChildFolderPath(parent_id, parent_uid, child_uid) + "\\" + filename;
    std::string abs_path = toAbsolutePath(full_path);
    std::replace(abs_path.begin(), abs_path.end(), '\\', '/');

    // [5] 실제로 .wav 파일 저장
    if (!SaveBinaryFile(full_path, payload)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"파일 저장 실패";
        cout << u8"→[SaveBinaryFile] 실행 실패" << endl;
        return response;
    }
    cout << u8"→[SaveBinaryFile] 파일 저장 성공" << endl;

    // [6] 파일 저장
    std::string command = "venv\\Scripts\\python.exe embed_child_voice.py \"" + full_path + "\" " + std::to_string(child_uid);
    FILE* pipe = _popen(command.c_str(), "r");
    
    cout << u8"[DEBUG] full_path: " << full_path << endl;
    cout << u8"[DEBUG] abs_path : " << abs_path << endl;
    cout << u8"[DEBUG] 실행 명령: " << command << endl;

    if (!pipe) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"임베딩 실행 실패";
        cout << u8"→[EMBEDDING] 실행 실패" << endl;
        return response;
    }

    char buffer[4096];
    std::string json_output;

    // 먼저 파이썬 출력 받아오기
    while (fgets(buffer, sizeof(buffer), pipe)) {
        json_output += buffer;
    }
    _pclose(pipe);

    // 이제 JSON 블록만 추출
    std::size_t start = json_output.find('{');
    std::size_t end = json_output.rfind('}');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        json_output = json_output.substr(start, end - start + 1);
    }
    else {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"JSON 파싱 실패: 출력이 올바른 JSON이 아님";
        return response;
    }
    // 그대로 저장 (파싱 안 함)
    string error_msg;
    if (!db.setVoiceVectorRaw(child_uid, json_output, error_msg)) {
        cout << u8"[ERROR] DB 저장 실패: " << error_msg << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"DB 저장 실패: " + toUTF8_safely(error_msg);
        cout << u8"→[setVoiceVectorRaw] DB 저장 실패" << endl;

        return response;
    }
    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"임베딩 및 저장 완료";
    cout << u8"→[EMBEDDING] 임베딩 및 저장 완료" << endl;
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

