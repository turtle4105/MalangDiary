#include "DBManager.h"
#include <iostream>
using namespace std;

DBManager::DBManager() {}
DBManager::~DBManager() {}

//============ [DB 연결] ============
bool DBManager::connect() {
    try {
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://10.10.20.112:3306/malang_diary");

        sql::Properties properties({
            {"user", "seonseo"},
            {"password", "1234"}
            });

        conn_.reset(driver->connect(url, properties));
        return true;

    }
    catch (sql::SQLException& e) {
        cerr << " {{(>_<)}} DB 연결 실패: " << e.what() << endl;
        return false;
    }
}

//============ [회원가입] ============
/*    id, pw, name, phone number     */
bool DBManager::registerParents(
    const string& id, const string& pw,
    const string& nickname, const string& phone,
    int& out_parents_uid, string& out_error_msg) 
{
    if (!conn_) {
        out_error_msg = "→ [DB 오류] DB 연결 실패";
        return false;
    }

    try {
        //[1] ID 중복 확인 쿼리 준비 PreparedStatement -> ? 바인딩 파라미터 처리
        std::unique_ptr<sql::PreparedStatement> checkID(
            conn_->prepareStatement("SELECT COUNT(*) FROM parents WHERE id = ?")
        ); //unique_ptr을 사용하여 자원을 자동 해제 처리함, conn_은 이미 연결된 DB객체
        //바인딩 값 세팅함~ 쿼리문의 실제 값과 순서 일치하도록 값을 바인딩

        checkID->setString(1, id);
        std::unique_ptr<sql::ResultSet> res(checkID->executeQuery()); //여기서 쿼리 실행합니다.

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
        std::unique_ptr<sql::PreparedStatement> check_UID(
            conn_->prepareStatement("SELECT LAST_INSERT_ID()")
        );
        std::unique_ptr<sql::ResultSet> uidRes(check_UID->executeQuery());

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

//============ [로그인] ============
/*    id, pw -> out_parents_uid, out_nickname, out_lst_children    */
bool DBManager::login(
    const string& id, const string& pw,  
    int& out_parents_uid,
    string& out_nickname,
    vector<std_child_info>& out_children,
    string& out_error_msg)
{
    if (!conn_) {
        out_error_msg = "→[DB 오류] DB 연결 실패";
        return false;
    }

    try {
        // [1] ID + PW check_login
        std::unique_ptr<sql::PreparedStatement> check_login(
            conn_->prepareStatement("SELECT parents_uid, nickname FROM parents WHERE id = ? AND pw = ?")
        );
        check_login->setString(1, id);
        check_login->setString(2, pw);
        std::unique_ptr<sql::ResultSet> res(check_login->executeQuery());

        if (!res->next()) {
            out_error_msg = "아이디 또는 비밀번호가 일치하지 않습니다.";
            return false;
        }

        // [2] 정보 가져오기
        out_parents_uid = res->getInt("Parents_uid");
        out_nickname = res->getString("nickname");

        // [3] 해당 부모의 자녀 정보 가져오기
        std::unique_ptr<sql::PreparedStatement> childStmt(
            conn_->prepareStatement("SELECT child_uid, name, gender, birth_date, icon_color FROM child WHERE parents_id = ?")
        );
        childStmt->setInt(1, out_parents_uid);
        std::unique_ptr<sql::ResultSet> childRes(childStmt->executeQuery());

        while (childRes->next()) {
            std_child_info child;
            child.child_uid = childRes->getInt("child_uid");
            child.name = childRes->getString("name");
            child.gender = childRes->getString("gender");
            child.birth = childRes->getString("birth_date");
            child.icon_color = childRes->getInt("icon_color");
            out_children.push_back(child);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        cerr << "→ [DB 오류] 로그인 실패: " << e.what() << endl;
        out_error_msg = "{{{(>_<)}}} DB오류_예외적인 경우 .. 확인필요 ";
        return false; //결과 반환
    }
}

