//#include "ProtocolHandler.h"
//#include "DBManager.h"
//#include <iostream>
#include "ProtocolHandler.h"
#include "DBManager.h"
#include <iostream>
#include <windows.h>  // toUTF8 함수에 필요
//using namespace std;
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
//
//wstring Utf8ToWstring(const string& str) {
//    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
//    if (len == 0) return L"";
//    wstring result(len, L'\0');
//    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
//    result.pop_back();  // null 제거
//    return result;
//}
//
//// =======================================================================
//// [회원가입 handler]
//// 클라이언트로부터 받은 회원가입 요청 JSON을 파싱하고,
//// DBManager를 통해 부모 정보를 DB에 삽입한 뒤,
//// 성공 여부에 따라 응답 JSON을 구성하여 반환하는 함수.
//// =======================================================================
//nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
//
//    // 응답 JSON 객체 생성
//    nlohmann::json response;
//    response["PROTOCOL"] = "REGISTER_PARENTS";  // 요청 프로토콜 이름을 명시 
//
//    // DB 처리 결과를 저장할 변수들
//    int uid = -1;               // 부모 UID (가입 성공 시 할당됨)
//    string error_msg;      // 실패 시 전달할 에러 메시지
//
//    // DBManager를 통해 실제 회원가입 시도
//    // json에서 ID, PW, NICKNAME, PHONE_NUMBER를 추출하여 넘김
//    bool result = db.registerParents(
//        json.value("ID", ""),
//        json.value("PW", ""),
//        json.value("NICKNAME", ""),
//        json.value("PHONE_NUMBER", ""),
//        uid, error_msg
//    );
//
//    cout << "[DEBUG] 받은 JSON 구조:\n" << json.dump(2) << endl;
//    cout << "ID: " << json.value("ID", "null") << endl;
//    cout << "PW: " << json.value("PW", "null") << endl;
//    cout << "NICKNAME: " << json.value("NICKNAME", "null") << endl;
//    cout << "PHONE_NUMBER: " << json.value("PHONE_NUMBER", "null") << endl;
//
//
//
//    if (result) {
//        // [성공] 회원가입이 성공적으로 이루어졌을 때 응답 구성
//        response["RESP"] = "SUCCESS";           // 응답 상태
//        response["MESSAGE"] = "가입 완료";      // 사용자에게 전달할 메시지
//        response["PARENTS_UID"] = uid;          // DB에서 생성된 부모 UID
//    }
//    else {
//        // [실패] DB 삽입 실패 또는 ID 중복 등의 이유
//        response["RESP"] = "FAIL";              // 응답 상태
//        response["MESSAGE"] = toUTF8_safely(error_msg);
//        // 상세 실패 이유
//
//        wcout << Utf8ToWstring(response.dump(2)) << endl;
//
//
//
//    }
//
//    // 최종 응답 JSON 반환
//    return response;
//}
//
//




// =======================================================================
// [회원가입 handler]
// 클라이언트로부터 받은 회원가입 요청 JSON을 파싱하고,
// DBManager를 통해 부모 정보를 DB에 삽입한 뒤,
// 성공 여부에 따라 응답 JSON을 구성하여 반환하는 함수.
// =======================================================================
nlohmann::json ProtocolHandler::handle_RegisterParents(const nlohmann::json& json, DBManager& db) {
    // 디버깅 출력 (CP949 기준 콘솔용 출력)
    cout << u8"[REGISTER_PARENTS] 요청:\n" << json.dump(2) << endl;

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
        response["MESSAGE"] = u8"가입 완료";  // UTF-8 클라이언트용
        response["PARENTS_UID"] = uid;

        cout << u8"→ [REGISTER_PARENTS] 회원가입 성공 - UID: " << uid << endl;
        cout << u8"[REGISTER_PARENTS] ID: " << json.value("ID", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PW: " << json.value("PW", "null") << endl;
        cout << u8"[REGISTER_PARENTS] NICKNAME: " << json.value("NICKNAME", "null") << endl;
        cout << u8"[REGISTER_PARENTS] PHONE_NUMBER: " << json.value("PHONE_NUMBER", "null") << endl;

    }
    else {
        cout << u8"→ [REGISTER_PARENTS] 회원가입 실패 :" << endl;

        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}

nlohmann::json ProtocolHandler::handle_Login(const nlohmann::json& json, DBManager& db) {
    nlohmann::json response;
    response["PROTOCOL"] = "LOGIN";

    // [1] 받은 JSON 로그 출력
    cout << u8"[LOGIN] 요청:\n" << json.dump(2) << endl;
    cout << u8"[LOGIN] ID: " << json.value("ID", "null") << endl;
    cout << u8"[LOGIN] PW: " << json.value("PW", "null") << endl;

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
        cout << u8"→ [LOGIN] 로그인 실패: " << error_msg << endl;
        response["RESP"] = "FAIL";
        response["MESSAGE"] = toUTF8_safely(error_msg);
    }

    return response;
}
