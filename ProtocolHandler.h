#pragma once
#include <nlohmann/json.hpp>
class DBManager;

class ProtocolHandler {
public:
    //회원가입
    static nlohmann::json handle_RegisterParents(const nlohmann::json& json, DBManager& db);
    //로그인
    static nlohmann::json handle_Login(const nlohmann::json& json, DBManager& db);
    //아이등록
    //static nlohmann::json handle_RegisterChild(const nlohmann::json& json, DBManager& db);

};

/*“헤더로 길이 파악 → json 파싱 → 프로토콜 분기 → DBManager로 로직 실행
→ 결과를 ProtocolHandler로 전달 → 응답 JSON 구성 → send로 클라이언트에 반환”*/