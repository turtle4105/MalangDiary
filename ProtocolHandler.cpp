#include "ProtocolHandler.h"
#include "DBManager.h"
#include "TcpServer.h"
#include "FileUtils.h"   
#include "DiaryGenerator.h"
#include <sstream>   // << 이거 반드시!
#include <iomanip>   // std::put_time 사용을 위해 필요
#include <ctime>
#include <iostream>
#include <fstream>
#include <windows.h>  // toUTF8 함수에 필요
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
        response["CHILDREN"] = children_array;
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
    cout << response.dump(2) << endl;
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] 요청:\n" << json.dump(2) << endl;

    // [1] 관련 변수 선언
    // - 일기
    string title;
    string weather;
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

// [특정 일기 조회 handler]
void ProtocolHandler::handle_SendDiaryDetail(const nlohmann::json& json, DBManager& db, SOCKET clientSocket) {
    nlohmann::json response;
    response["PROTOCOL"] = "SEND_DIARY_DETAIL";
    cout << u8"[SEND_DIARY_DETAIL] 요청:\n" << json.dump(2) << endl;

    // [1] 필드 선언
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

    // [2] 필수 파라미터 확인
    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"DIARY_UID 누락됨";
        TcpServer::sendJsonResponse(clientSocket, response.dump());
        return;
    }

    // [3] DB에서 일기 상세 조회 (voice_path 포함되도록 getDiaryDetailByUID 수정 필요)
    if (!db.getDiaryDetailByUID(diary_uid, title, text, weather, is_liked, photo_path, voice_path, create_at, emotions, out_error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = out_error_msg;
        TcpServer::sendJsonResponse(clientSocket, response.dump());
        return;
    }

    // [4] 감정 배열 구성
    nlohmann::json emotion_array = nlohmann::json::array();
    for (const auto& emo : emotions) {
        emotion_array.push_back({ {"EMOTION", emo} });
    }

    // [5] JSON 응답 데이터 구성
    response["RESP"] = "SUCCESS";
    response["DIARY_UID"] = diary_uid;
    response["TITLE"] = title;
    response["TEXT"] = text;
    response["WEATHER"] = weather;
    response["IS_LIKED"] = is_liked;
    response["PHOTO_PATH"] = photo_path;
    response["CREATE_AT"] = create_at;
    response["EMOTIONS"] = emotion_array;
    response["MESSAGE"] = u8"조회 성공";

    // [6] 음성 파일 경로 생성 및 로드
    std::vector<char> voice_payload;

    if (!voice_path.empty()) {
        // [1] 앞에 '/' 제거 (상대 경로로 만들기)
        std::string cleanVoicePath = (voice_path[0] == '/') ? voice_path.substr(1) : voice_path;

        // [2] 절대 경로 생성
        std::string abs_voice_path =  cleanVoicePath;
        std::replace(abs_voice_path.begin(), abs_voice_path.end(), '/', '\\');

        // [3] 파일 로드 시도
        std::ifstream voice_file(abs_voice_path, std::ios::binary);
        if (voice_file) {
            voice_payload.assign(std::istreambuf_iterator<char>(voice_file), std::istreambuf_iterator<char>());
            cout << u8"→ [SEND_DIARY_DETAIL] 음성 파일 로드 완료 (" << voice_payload.size() << " bytes)" << endl;
        }
        else {
            cerr << u8"[오류] 음성 파일 열기 실패: " << abs_voice_path << endl;
        }
    }
    else {
        cout << u8"[INFO] voice_path 비어 있음. 음성 파일 전송 생략됨." << endl;
    }

    // [7] 메타 JSON + 음성 payload 전송
    if (voice_payload.empty()) {
        TcpServer::sendJsonResponse(clientSocket, response.dump());
    }
    else {
        TcpServer::sendPacket(clientSocket, response.dump(), voice_payload);
    }

    // [8] 사진이 존재할 경우 두 번째 전송
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
            img_json["MESSAGE"] = u8"이미지 포함 전송";

            TcpServer::sendPacket(clientSocket, img_json.dump(), payload);
            cout << u8"→ [SEND_DIARY_IMG] 사진 전송 완료 (" << payload.size() << " bytes)\n";
        }
        else {
            cerr << u8"[오류] 사진 파일 열기 실패: " << abs_path << endl;
        }
    }

    // [9] 디버깅용 출력
    cout << response.dump(2) << endl;
}
// [일기 삭제 handler]
nlohmann::json ProtocolHandler::handle_DiaryDel(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    cout << u8"[DIARY_DEL] 요청:\n" << json.dump(2) << endl;

    response["PROTOCOL"] = "DIARY_DEL";
    string error_msg;

    int diary_uid = json.value("DIARY_UID", -1);
    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "DIARY_UID 누락됨";
        return response;
    }
    if (!db.deleteDiaryByUid(diary_uid, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"→ [DIARY_DEL] 일기 삭제 실패" << endl;

        return response;
    }

    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"일기 삭제 완료";
    cout << u8"→ [DIARY_DEL] 일기 삭제 완료" << endl;

    return response;

}

// [일기 좋아요 변경 handler]
nlohmann::json ProtocolHandler::handle_UpdateLike(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    cout << u8"[UPDATE_DIARY_LIKE] 요청:\n" << json.dump(2) << endl;

    response["PROTOCOL"] = "UPDATE_DIARY_LIKE";
    string error_msg;

    int diary_uid = json.value("DIARY_UID", -1);
    int is_liked = json.value("IS_LIKED", -1);

    if (diary_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "DIARY_UID 누락됨";
        return response;
    }

    if (!db.Update_DiaryLiked(diary_uid, is_liked, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        cout << u8"→ [UPDATE_DIARY_LIKE] 좋아요 여부 변경 실패" << endl;

        return response;
    }

    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"좋아요 여부 변경 완료";
    cout << u8"→ [DIARY_DEL] 좋아요 여부 변경 완료" << endl;

    return response;

}

// [일기 수정 handler]
nlohmann::json ProtocolHandler::handle_ModifyDiary(const nlohmann::json& json, const vector<char>& payload, DBManager& db) {
    cout << u8"[MODIFY_DIARY] 요청:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "MODIFY_DIARY";

    // [1] 필드 파싱
    int child_uid = json.value("CHILD_UID", -1);
    int diary_uid = json.value("DIARY_UID", -1);
    std::string title = json.value("TITLE", "");
    std::string text = json.value("TEXT", "");
    int is_liked = json.value("IS_LIKED", 0);

    std::string weatherStr = json.value("WEATHER", "");

    std::string photo_filename = json.value("PHOTO_FILENAME", "");
    std::string create_at = json.value("CREATE_AT", "");

    // [2] 감정 추출
    std::vector<std::string> emotions;
    for (const auto& e : json.value("EMOTIONS", nlohmann::json::array())) {
        emotions.push_back(e.value("EMOTION", ""));
    }

    // [3] 부모 ID 조회
    int parent_uid;
    std::string parent_id;
    if (!db.getParentsUidByChild(child_uid, parent_uid) ||
        !db.getParentIdByUID(parent_uid, parent_id)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"부모 정보 조회 실패";
        return response;
    }

    // [4] 경로 생성
    std::string abs_path = GetChildFolderPath(parent_id, parent_uid, child_uid) + "\\image\\" + photo_filename;
    std::string photo_path = "/" + parent_id + "_" + std::to_string(parent_uid) + "/" +
        std::to_string(child_uid) + "/image/" + photo_filename;
    std::replace(abs_path.begin(), abs_path.end(), '/', '\\');

    // [5] 사진 저장 (payload가 있을 경우만)
    if (!payload.empty()) {
        cout << u8"→ [사진 저장 경로] " << abs_path << endl;
        if (!SaveBinaryFile(abs_path, payload)) {
            response["RESP"] = "FAIL";
            response["MESSAGE"] = u8"사진 저장 실패";
            return response;
        }
    }

    else {
        cout << u8"→ [사진 저장 건너뜀] payload 없음" << endl;
    }

    // [6] DB 수정
    std::string error_msg;
    bool result = db.Modify_diary(diary_uid, title, text, weatherStr, is_liked, create_at, emotions, photo_path, error_msg);

    if (!result) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // [7] 성공 응답
    response["RESP"] = "SUCCESS";
    response["MESSAGE"] = u8"일기 수정 성공";

    cout << u8"→ [MESSAGE] 일기 수정 반영 완료" << endl;
    cout << response.dump(2);
    return response;
}
// [달력 조회 handler]
nlohmann::json ProtocolHandler::handle_Calendar(const nlohmann::json& json, DBManager& db) {
    cout << u8"[handle_Calendar] 요청:\n" << json.dump(2) << endl;

    nlohmann::json response;
    response["PROTOCOL"] = "SEND_CALENDAR_REQ";

    // [1] 필드 파싱
    int child_uid = json.value("CHILD_UID", -1);
    int year = json.value("YEAR", -1);
    int month = json.value("MONTH", -1);

    if (child_uid == -1 || year == -1 || month == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID 또는 날짜 정보 누락됨";
        return response;
    }

    // [2] DB 호출
    std::vector<CalendarData> calendar;
    std::string error_msg;

    if (!db.getCalendarData(child_uid, year, month, calendar, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }
    // [3] 응답 데이터 구성
    nlohmann::json data_array = nlohmann::json::array();
    for (const auto& entry : calendar) {
        data_array.push_back({
            {"DIARY_UID", entry.diary_uid},
            {"DATE", entry.date},  // 정수형 일(day)
            {"IS_WRITED", entry.is_writed},
            {"IS_LIKED", entry.is_liked}
            });
    }
    response["RESP"] = "SUCCESS";
    response["DATA"] = data_array;
    cout << u8"→ [SEND_CALENDAR_REQ] 달력 조회 완료" << endl;

    return response;
}

nlohmann::json ProtocolHandler::handle_GenDiary(const nlohmann::json& json, const std::vector<char>& payload, DBManager& db) {
    cout << u8"[handle_GenDiary] 요청:\n" << json.dump(2) << endl;
    nlohmann::json response;
    response["PROTOCOL"] = "GEN_DIARY_RESULT";

    // 요청 필드 파싱 ~ DB꺼
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

    // 음성 저장 단계
    nlohmann::json saveResp = handle_GenDiary_SaveVoice(json, payload, db);
    if (saveResp.value("RESP", "") != "SUCCESS") {
        return saveResp; //실패 메시지 그대로 클라한테 주려고
    }
    cout << u8"[handle_GenDiary-1] 음성 저장 성공. VOICE_PATH: " << saveResp.value("VOICE_PATH", "") << endl;
    // 요청 필드 파싱 ~ 파이썬꺼
    string audio_path = saveResp.value("VOICE_PATH", "");
    string embedding_data;

    bool getVoiceVector_result = db.getVoiceVector(child_uid, embedding_data, out_error_msg);

    cout << "[handle_GenDiary] getVoiceVector_result: " << getVoiceVector_result << endl;

    cout << u8"[handle_GenDiary-2] DB에서 임베딩 벡터 불러오는 중..." << endl;
    if (!getVoiceVector_result) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"임베딩 DB 로딩 실패: " + out_error_msg;
        return response;
    }

    string embedding_only_str;
    if (!DiaryGenerator::extract_embedding_array(embedding_data, embedding_only_str, out_error_msg)) {  // 괄호 닫힘, 세미콜론 추가
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"임베딩 문자열 변환 실패: " + out_error_msg;
        return response; // return 누락되었음
    }

    cout << u8"[handle_GenDiary-3] 임베딩 불러오기 성공, 길이: " << embedding_data.size() << " bytes" << endl;

    if (child_uid == -1 || child_name.empty() || audio_path.empty() || embedding_data.empty()) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"필수 필드 누락 (CHILD_UID, CHILD_NAME, AUDIO_PATH, EMBEDDING)";
        return response;
    }
    cout << u8"[handle_GenDiary-4] 파라미터 유효성 통과" << endl;

    // Python 서버에 전송
    cout << u8"[handle_GenDiary-5] Python 서버 호출 시작" << endl;

    nlohmann::json diary_data;
    string error_msg;
    if (!DiaryGenerator::sendToPythonDiaryServer(audio_path, embedding_only_str, child_name, diary_data, error_msg)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = error_msg;
        return response;
    }
    cout << u8"[handle_GenDiary-6] Python 응답 성공\n→ title: " << diary_data.value("title", "");
    // 오늘 날짜 얻기

    // 감정 리스트 파싱
    std::vector<std::string> emotion_list;
    for (const auto& e : diary_data["emotion"]) {
        emotion_list.push_back(e);
    }

    bool insert_result = db.insertGeneratedDiary(
        child_uid,
        diary_data.value("title", ""),
        diary_data.value("content", ""),
        diary_data.value("weather", ""),  // weather 초기값 또는 선택값으로 설정
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
    // 응답 구성
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
    cout << u8"[handle_GenDiary_SaveVoice] 실행" << endl;

    int child_uid = json.value("CHILD_UID", -1);
    string child_name = json.value("NAME", "");
    std::string filename;  // 나중에 날짜 기반으로 채울 예정

    cout << u8"[handle_GenDiary_SaveVoice] 필드 파싱 완료" << endl;

    if (child_uid < 0 || child_name.empty()) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"CHILD_UID 또는 NAME 누락";
        return response;
    }
    
    // 2) 부모 ID → parentId 조회
    int parents_uid;
    std::string parentId;
    if (!db.getParentsUidByChild(child_uid, parents_uid) ||
        !db.getParentIdByUID(parents_uid, parentId))
    {   
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"부모 정보 조회 실패";
        return response;
    }
    cout << u8"[handle_GenDiary_SaveVoice -1] 부모 정보 조회" << endl;

    // 3) child voice 디렉토리 경로

    cout << u8"parentsuid"<<parents_uid<<"parentsid" << parentId << "child_uid" << child_uid << endl;

    std::string childPath = GetChildFolderPath(parentId, parents_uid, child_uid);
    std::string voiceDir = childPath + "\\voice";

    // 4) 오늘 날짜 기반 파일명 생성
    std::time_t t = std::time(nullptr);
    std::tm    tm;
    localtime_s(&tm, &t);
    {
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d") << ".wav";
        filename = oss.str();
    }
    // 5) 전체 저장 경로
    std::string fullPath = voiceDir + "\\" + filename;

    // 6) 파일 저장
    if (!SaveBinaryFile(fullPath, payload)) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = u8"음성 파일 저장 실패";
        return response;
    }
    cout << u8"[handle_GenDiary_SaveVoice-2] 음성 파일 저장" << endl;


    // **여기서 성공 리턴**
    response["RESP"] = "SUCCESS";
    response["VOICE_PATH"] = fullPath;
    cout << u8"→ [VOICE_PATH] 음성파일 저장 완료!! >0<" << endl;
    cout << "handle_GenDiary_SaveVoice() Done" << endl;

    return response;
}

