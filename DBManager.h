#pragma once
#include <mariadb/conncpp.hpp>
#include <memory>
#include <string>
#include <vector>            

using namespace std;

struct std_child_info {
    int child_uid;
    string name;
    string gender;
    string birth;
    int icon_color;
}; 

class DBManager {
public:
    DBManager();
    ~DBManager();

    // DB 연결
    bool connect();

    // 회원 가입
    bool registerParents(const string& id, const string& pw,
        const string& nickname, const string& phone,
        int& out_parents_uid, string& out_error_msg);
 
    // 로그인 처리
    bool login(const string& id, const string& pw,
        int& out_parents_uid, string& out_nickname,
        vector<std_child_info>& out_children, string& out_error_msg);

    // 자녀 등록
    bool registerChild(const int& parents_uid, const string& name,
        const string& birthdate, const string& gender,
        const string& icon_color, int& out_child_uid, string& out_error_msg);

    // 부모 아이디 조회
    bool getParentIdByUID(int uid, string& out_id);

    // 초기화면 일기조회
    bool getLatestDiary(const int& child_uid,
        string& title,
        int& weather,
        string& date,
        vector<string>& emotions, string& out_error_msg);



private:
    unique_ptr<sql::Connection> conn_;
};
