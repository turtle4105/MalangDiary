#include "DBManager.h"
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;

DBManager::DBManager() {}
DBManager::~DBManager() {}

//============ [DB 연결] ============
bool DBManager::connect() {
    try {
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        /*sql::SQLString url("jdbc:mariadb://10.10.20.112:3306/malang_diary?useSSL=false");*/
        sql::SQLString url("jdbc:mariadb://10.10.20.116:3306/malang_diary?sslMode=DISABLED");

        sql::Properties properties({
            {"user", "root"},
            {"password", "1234"} ,
        {"sslMode", "DISABLED"}
            });

        conn_.reset(driver->connect(url, properties));
        return true;

    }
    catch (sql::SQLException& e) {
        cerr << " {{(>_<)}} DB 연결 실패: " << e.what() << endl;
        return false;
    }
}

// ========== [파일 생성 디렉토리 서치] ============
bool DBManager::getParentIdByUID(int uid, string& out_id) {
    try {
        unique_ptr<sql::PreparedStatement> stmt(
            conn_->prepareStatement("SELECT id FROM parents WHERE parents_uid = ?")
        );
        stmt->setInt(1, uid);
        unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) {
            out_id = res->getString("id");
            return true;
        }
    }
    catch (sql::SQLException& e) {
        cerr << "→ [DB 오류] 부모 ID 조회 실패: " << e.what() << endl;
    }
    return false;
}

bool DBManager::getParentsUidByChild(int child_uid, int& out_parents_uid) {
    if (!conn_) return false;
    try {
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn_->prepareStatement("SELECT parents_id FROM child WHERE child_uid = ?")
        );
        stmt->setInt(1, child_uid);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) {
            out_parents_uid = res->getInt("parents_id");
            return true;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << u8"[DB 오류] 부모 UID 조회 실패: " << e.what() << std::endl;
    }
    return false;
}

//bool DBManager::getChildNameByUID(int child_uid, std::string& out_name) {
//    if (!conn_) return false;
//    try {
//        std::unique_ptr<sql::PreparedStatement> stmt(
//            conn_->prepareStatement("SELECT name FROM child WHERE child_uid = ?")
//        );
//        stmt->setInt(1, child_uid);
//        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
//
//        if (res->next()) {
//            out_name = res->getString("name");
//            return true;
//        }
//    }
//    catch (sql::SQLException& e) {
//        std::cerr << "[DB 오류] 자녀 이름 조회 실패: " << e.what() << std::endl;
//    }
//    return false;
//}

//bool DBManager::setVoicePath(int child_uid, const string& path) {
//    if (!conn_) {
//        return false;
//    }
//    try {
//        std::unique_ptr<sql::PreparedStatement> stmt(
//            conn_->prepareStatement("UPDATE child SET setting_path = ? WHERE child_uid = ?")
//        );
//        stmt->setString(1, path);
//        stmt->setInt(2, child_uid);
//        stmt->executeUpdate();
//        return true;
//    }
//    catch (sql::SQLException& e) {
//        return false;
//    }
//}
//

bool DBManager::setVoiceVectorRaw(int child_uid, const std::string& jsonStr, std::string& out_error) {
    if (!conn_) {
        out_error = u8"DB 연결 안됨";
        return false;
    }

    try {
        // JSON 재구성 → 깨질 수 있는 인코딩 요소 제거
        nlohmann::json parsed_json = nlohmann::json::parse(jsonStr);
        std::string cleaned_json = parsed_json.dump();  // 인코딩-safe하게 다시 dump

        std::unique_ptr<sql::PreparedStatement> stmt(
            conn_->prepareStatement("UPDATE child SET voice_vector = ? WHERE child_uid = ?")
        );
        stmt->setString(1, cleaned_json);
        stmt->setInt(2, child_uid);
        stmt->executeUpdate();
        return true;
    }
    catch (const nlohmann::json::parse_error& e) {
        out_error = string(u8"JSON 파싱 실패: ") + e.what();
        return false;
    }
    catch (sql::SQLException& e) {
        out_error = e.what();
        return false;
    }
}

//    ============ [회원가입] ============
/*    id, pw, name, phone number     */
bool DBManager::registerParents(
    const string& id, const string& pw,
    const string& nickname, const string& phone,
    int& out_parents_uid, string& out_error_msg) {
    if (!conn_) {
        out_error_msg = "→ [DB 오류] DB 연결 실패";
        return false;
    }

    try {
        //[1] ID 중복 확인 쿼리 준비 PreparedStatement -> ? 바인딩 파라미터 처리
        unique_ptr<sql::PreparedStatement> checkID(
            conn_->prepareStatement("SELECT COUNT(*) FROM parents WHERE id = ?")
        ); //unique_ptr을 사용하여 자원을 자동 해제 처리함, conn_은 이미 연결된 DB객체
        //바인딩 값 세팅함~ 쿼리문의 실제 값과 순서 일치하도록 값을 바인딩

        checkID->setString(1, id);
        unique_ptr<sql::ResultSet> res(checkID->executeQuery()); //여기서 쿼리 실행합니다.

        if (res->next() && res->getInt(1) > 0) {
            out_error_msg = "이미 존재하는 아이디입니다.";
            return false;
        }

        //[2] INSERT 
        unique_ptr<sql::PreparedStatement> insert(
            conn_->prepareStatement("INSERT INTO parents (id, pw, nickname, phone_number) VALUES (?, ?, ?, ?)")
        );
        insert->setString(1, id);
        insert->setString(2, pw);
        insert->setString(3, nickname);
        insert->setString(4, phone);
        insert->executeUpdate();

        // [3] UID 가져오기
        unique_ptr<sql::PreparedStatement> check_UID(
            conn_->prepareStatement("SELECT LAST_INSERT_ID()")
        );
        unique_ptr<sql::ResultSet> uidRes(check_UID->executeQuery());

        if (uidRes->next()) {
            out_parents_uid = uidRes->getInt(1);  // 첫 번째 컬럼
            //cout << u8"→ [회원가입 승인] ID: " << id << ", UID: " << out_parents_uid << endl;
            return true;
        }
        else {
            out_error_msg = "UID 조회 실패";
            return false;
        }
    }
    catch (sql::SQLException& e) {
        cerr << u8"→ [DB 오류] 회원가입 실패: " << e.what() << endl;
        out_error_msg = "{{{(>_<)}}} DB오류_예외적인 경우 .. 확인필요 ";
        return false; //결과 반환
    }
}

//    ============ [로그인] ============
/*    id, pw -> out_parents_uid, out_nickname, out_lst_children    */
bool DBManager::login(
    const string& id, const string& pw,  
    int& out_parents_uid,
    string& out_nickname,
    vector<std_child_info>& out_children,
    string& out_error_msg){
    if (!conn_) {
        out_error_msg = "→[DB 오류] DB 연결 실패";
        return false;
    }

    try {
        // [1] ID + PW check_login
        unique_ptr<sql::PreparedStatement> check_login(
            conn_->prepareStatement("SELECT parents_uid, nickname FROM parents WHERE id = ? AND pw = ?")
        );
        check_login->setString(1, id);
        check_login->setString(2, pw);
        unique_ptr<sql::ResultSet> res(check_login->executeQuery());

        if (!res->next()) {
            out_error_msg = "아이디 또는 비밀번호가 일치하지 않습니다.";
            return false;
        }

        // [2] 정보 가져오기
        out_parents_uid = res->getInt("Parents_uid");
        out_nickname = res->getString("nickname");

        // [3] 해당 부모의 자녀 정보 가져오기
        unique_ptr<sql::PreparedStatement> childStmt(
            conn_->prepareStatement("SELECT child_uid, name, gender, birth_date, icon_color FROM child WHERE parents_id = ?")
        );
        childStmt->setInt(1, out_parents_uid);
        unique_ptr<sql::ResultSet> childRes(childStmt->executeQuery());

        while (childRes->next()) {
            std_child_info child;
            child.child_uid = childRes->getInt("child_uid");
            child.name = childRes->getString("name");
            child.gender = childRes->getString("gender");
            child.birth = childRes->getString("birth_date");
            child.icon_color = childRes->getString("icon_color");
            out_children.push_back(child);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << u8"→ [DB 오류] 로그인 실패: " << e.what() << endl;
        out_error_msg = "{{{(>_<)}}} DB오류_예외적인 경우 .. 확인필요 ";
        return false; //결과 반환
    }
}

//    ============ [자녀 등록] ============
/*    parents_uid, name, birthdate, gender,icon_color   -> child_uid   */
bool DBManager::registerChild(
    const int& parents_uid,
    const std::string& name,
    const std::string& birthdate,
    const std::string& gender,
    std::vector<std_child_info>& out_children,
    int& out_child_uid,
    const std::string& icon_color,
    std::string& out_error_msg
) {
    
    out_child_uid = -1;
    out_children.clear();

    if (!conn_) {
        out_error_msg = u8"→[DB 오류] DB 연결 실패";
        return false;
    }
    try {
        // [0] 부모 내 중복 이름 체크
        unique_ptr<sql::PreparedStatement> checkDup(
            conn_->prepareStatement("SELECT COUNT(*) FROM child WHERE parents_id = ? AND name = ?")
        );
        checkDup->setInt(1, parents_uid);
        checkDup->setString(2, name);
        unique_ptr<sql::ResultSet> dupRes(checkDup->executeQuery());

        if (dupRes->next() && dupRes->getInt(1) > 0) {
            out_error_msg = u8"같은 이름의 자녀가 이미 등록되어 있습니다.";
            return false;
        }

        // [1] INSERT
        unique_ptr<sql::PreparedStatement> insert(
            conn_->prepareStatement("INSERT INTO child ( parents_id, name, birth_date, gender, icon_color) VALUES (?, ?, ?, ?, ?)")
        );
        insert->setInt(1, parents_uid);
        insert->setString(2, name);
        insert->setString(3, birthdate);
        insert->setString(4, gender);
        insert->setString(5, icon_color);
        insert->executeUpdate();  

 
        // [3] UID 가져오기
        unique_ptr<sql::PreparedStatement> check_UID(
            conn_->prepareStatement("SELECT LAST_INSERT_ID()")
        );
        unique_ptr<sql::ResultSet> uidRes(check_UID->executeQuery());

        if (uidRes->next()) {
            out_child_uid = uidRes->getInt(1);  // 첫 번째 컬럼
        }
        else {
            out_error_msg = u8"UID 조회 실패";
            return false;
        }
        // [3] 해당 부모의 자녀 정보 가져오기
        unique_ptr<sql::PreparedStatement> childStmt(
            conn_->prepareStatement("SELECT child_uid, name, gender, birth_date, icon_color FROM child WHERE parents_id = ?")
        );

        childStmt->setInt(1, parents_uid);
        unique_ptr<sql::ResultSet> childRes(childStmt->executeQuery());

        while (childRes->next()) {
            std_child_info child;
            child.child_uid = childRes->getInt("child_uid");
            child.name = childRes->getString("name");
            child.gender = childRes->getString("gender");
            child.birth = childRes->getString("birth_date");
            child.icon_color = childRes->getString("icon_color");
            out_children.push_back(child);
        }
        return true;
    }

    catch (sql::SQLException& e) {
        cerr << "→ [DB 오류] 자녀 등록 실패: " << e.what() << endl;
        out_error_msg = u8"{{{(>_<)}}} DB오류_예외적인 경우 .. 확인필요 ";
        return false; 
    }
}

//    ============ [오늘의 일기 조회] ============
/*    child_uid -> title, weather, date, emotions   */
bool DBManager::getLatestDiary(const int& child_uid, int& diary_uid, string& photo_path, std::string& title,
    int& weather, std::string& date, std::vector<std::string>& emotions, std::string& out_error_msg) {

    if (!conn_) {
        out_error_msg = u8"→ [DB 오류] DB 연결 실패";
        return false;
    }

    try {
        // [1] 오늘 날짜를 "YYYYMMDD" 형식으로 얻기
        time_t now = time(nullptr);
        struct tm t;
        localtime_s(&t, &now);

        char buf[9];
        strftime(buf, sizeof(buf), "%Y%m%d", &t);
        std::string today = buf;

        // [2] 오늘 날짜의 일기 중 최신 1개 조회
        std::unique_ptr<sql::PreparedStatement> check_diary(
            conn_->prepareStatement(R"(
                SELECT diary_uid, title, weather, create_at, photo_path
                FROM diary
                WHERE child_id = ? AND is_deleted = 0 
                  AND DATE_FORMAT(create_at, '%Y%m%d') = ?
                ORDER BY create_at DESC
                LIMIT 1
            )")
        );

        check_diary->setInt(1, child_uid);
        check_diary->setString(2, today);

        std::unique_ptr<sql::ResultSet> res(check_diary->executeQuery());

        if (!res->next()) {
            out_error_msg = u8"오늘 날짜의 일기가 존재하지 않습니다.";
            return false;
        }

        diary_uid = res->getInt("diary_uid");
        title = res->getString("title");
        weather = res->getInt("weather");
        date = res->getString("create_at");
        photo_path = res-> getString("photo_path");

        // [3] 감정(emotions) 테이블에서 감정 정보 조회
        std::unique_ptr<sql::PreparedStatement> emo_stmt(
            conn_->prepareStatement(R"(
                SELECT emotion_1, emotion_2, emotion_3, emotion_4, emotion_5
                FROM emotions WHERE diary_uid = ?
            )")
        );
        emo_stmt->setInt(1, diary_uid);
        std::unique_ptr<sql::ResultSet> emo_res(emo_stmt->executeQuery());

        std::string fields[5] = { "emotion_1", "emotion_2", "emotion_3", "emotion_4", "emotion_5" };
        if (emo_res->next()) {
            for (const auto& field : fields) {
                std::string emotion = emo_res->getString(field).c_str();
                if (!emotion.empty()) {
                    emotions.push_back(emotion);
                }
            }
        }
        return true;
    }
    catch (sql::SQLException& e) {
        out_error_msg = u8"[DB 예외] " + std::string(e.what());
        return false;
    }
}

