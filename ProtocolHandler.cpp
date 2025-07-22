#include "ProtocolHandler.h"
#include "DBManager.h"
#include "FileUtils.h"        
#include <iostream>
#include <windows.h>  // toUTF8 함수에 필요
using namespace std;

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
    // 파싱한 값을 여기서 꺼냄 클라가 준 값
    int parents_uid = json.value("PARENTS_UID", -1);
    string babyname = json.value("NAME", "");
    string birthdate = json.value("BIRTHDATE", "");
    string gender = json.value("GENDER","");
    string icon_color = json.value("ICON_COLOR", "");
    string error_msg;

    bool result = db.registerChild(parents_uid, babyname, birthdate, gender, icon_color, out_child_uid, error_msg);

    if (result) {
        response["RESP"] = "SUCCESS";
        response["MESSAGE"] = u8"자녀 등록 성공";
        response["CHILD_UID"] = out_child_uid;

        cout << u8"→ [REGISTER_CHILD] 자녀 등록 성공 - UID: " << out_child_uid << endl;

        // 부모 ID 추가 조회
        string parentId;
        if (db.getParentIdByUID(parents_uid, parentId)) {
            if (CreateChildDirectory(parentId, parents_uid, babyname, out_child_uid)) {
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
        cout << u8"[REGISTER_CHILD] 자녀 등록 실패: "
             << toUTF8_safely(error_msg) << endl;
    }
    return response;
}

nlohmann::json ProtocolHandler::handle_LatestDiary(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "GET_LATEST_DIARY";
    cout << u8"[GET_LATEST_DIARY] 요청:\n" << json.dump(2) << endl;

    string title;
    int weather;
    string create_at;
    vector <string> emotions;
    string error_msg;

    int child_uid = json.value("CHILD_UID", -1);
    if (child_uid == -1) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = "CHILD_UID 누락됨";
        return response;
    }

    bool success = db.getLatestDiary(child_uid, title, weather, create_at, emotions, error_msg);
    if (!success) {
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
        return response;
    }

    // 성공 시 응답 구성
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

