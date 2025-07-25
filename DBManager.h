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

    // DB ����
    bool connect();

    // ȸ�� ����
    bool registerParents(const string& id, const string& pw,
        const string& nickname, const string& phone,
        int& out_parents_uid, string& out_error_msg);
 
    // �α��� ó��
    bool login(const string& id, const string& pw,
        int& out_parents_uid, string& out_nickname,
        vector<std_child_info>& out_children, string& out_error_msg);

    // �ڳ� ���
    bool registerChild(const int& parents_uid, const string& name,
        const string& birthdate, const string& gender, vector<std_child_info>& children,
        int& out_child_uid, const string& icon_color, string& out_error_msg);

    // �ʱ�ȭ�� �ϱ���ȸ
    bool getLatestDiary(const int& child_uid, int& diary_uid,
        string& title, string& photo_path,
        int& weather,
        string& date,
        vector<string>& emotions, string& out_error_msg);

    // �θ� ���̵� ��ȸ
    bool getParentIdByUID(int uid, string& out_id);

    // �ڳ� UID -> �θ� UID ��ȸ
    bool getParentsUidByChild(int child_uid, int& out_parents_uid);

    // �ڳ� UID -> �ڳ� �̸� ��ȸ
    //bool getChildNameByUID(int child_uid, string& out_name);

    // ���� �������� ��� ����
    //bool setVoicePath(int child_uid, const string& path);

    // ��Ҹ� ���� ����
    bool setVoiceVectorRaw(int child_uid, const string& jsonStr, string& out_error);

    // �ϱ� ��ȸ
    bool getDiaryDetailByUID(
        int diary_uid, string& title, string& text, int& weather,
        int& is_liked, string& photo_path, string& create_at, vector<string>& emotions, string& out_error_msg
    );

    // �ϱ� ����
    bool deleteDiaryByUid(int diary_uid, string& out_error_msg);

    // �ϱ� ���ƿ� ����
    bool Update_DiaryLiked(int diary_uid, int is_liked, string& out_error_msg);

    // �ϱ� ���� ����
    bool Modify_diary(int diary_uid,
        const string& title, const string& text,
        int weather, int is_liked,
        const string& create_at,
        const vector<string>& emotions,
        const string& photo_path,
        string& out_error_msg);

    // �޷� ��ȸ
    bool getCalendarData(int child_uid, int year, int month,
        std::vector<CalendarData>& out_data, string& out_error_msg);

    // �ϱ� ���� ���
    bool GenDiaryResult(int child_uid, string& name, int diary_uid,
        string& title, const vector<string>& emotions, string& out_error_msg);

    // �ϱ� ���� ����
    bool updateVoicePath(
        int child_uid, const string& voice_path, string& out_error_msg);
    
    // ���� ���� ��������
    bool getVoiceVector(int child_uid, string& out_vector_json, string& out_error_msg);

    bool getVoicePath(int child_uid, std::string& out_path, std::string& out_error_msg);




private:
    unique_ptr<sql::Connection> conn_;
};

