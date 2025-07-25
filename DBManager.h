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
    string icon_color;
};

struct CalendarData {
    int diary_uid;
    int date;
    bool is_writed;
    bool is_liked;
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
        const string& birthdate, const string& gender, vector<std_child_info>& children,
        int& out_child_uid, const string& icon_color, string& out_error_msg);

    // 초기화면 일기조회
    bool getLatestDiary(const int& child_uid, int& diary_uid,
        string& title, string& photo_path,
        int& weather,
        string& date,
        vector<string>& emotions, string& out_error_msg);

    // 부모 아이디 조회
    bool getParentIdByUID(int uid, string& out_id);

    // 자녀 UID -> 부모 UID 조회
    bool getParentsUidByChild(int child_uid, int& out_parents_uid);

    // 자녀 UID -> 자녀 이름 조회
    //bool getChildNameByUID(int child_uid, string& out_name);

    // 세팅 음성파일 경로 삽입
    //bool setVoicePath(int child_uid, const string& path);

    // 목소리 벡터 삽입
    bool setVoiceVectorRaw(int child_uid, const string& jsonStr, string& out_error);

    // 일기 조회
    bool getDiaryDetailByUID(
        int diary_uid, string& title, string& text, int& weather,
        int& is_liked, string& photo_path, string& create_at, vector<string>& emotions, string& out_error_msg
    );

    // 일기 삭제
    bool deleteDiaryByUid(int diary_uid, string& out_error_msg);

    // 일기 좋아요 변경
    bool Update_DiaryLiked(int diary_uid, int is_liked, string& out_error_msg);

    // 일기 수정 저장
    bool Modify_diary(int diary_uid,
        const string& title, const string& text,
        int weather, int is_liked,
        const string& create_at,
        const vector<string>& emotions,
        const string& photo_path,
        string& out_error_msg);

    // 달력 조회
    bool getCalendarData(int child_uid, int year, int month,
        std::vector<CalendarData>& out_data, string& out_error_msg);

    // 일기 생성 결과
    bool GenDiaryResult(int child_uid, string& name, int diary_uid,
        string& title, const vector<string>& emotions, string& out_error_msg);

    // 일기 음성 저장
    bool updateVoicePath(
        int child_uid, const string& voice_path, string& out_error_msg);
    
    // 음성 벡터 꺼내오기
    bool getVoiceVector(int child_uid, string& out_vector_json, string& out_error_msg);

    bool getVoicePath(int child_uid, std::string& out_path, std::string& out_error_msg);




private:
    unique_ptr<sql::Connection> conn_;
};

