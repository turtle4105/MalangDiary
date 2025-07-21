#pragma once
#include <mariadb/conncpp.hpp>
#include <memory>
#include <string>
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

    // DB ����
    bool connect();

    // ȸ�� ����
    bool registerParents(const string& id, const string& pw,
        const string& nickname, const string& phone,
        int& out_parents_uid, string& out_error_msg);
 
    // �α��� ó��
    bool login(const string& id,
        const string& pw,
        int& out_parents_uid,
        string& out_nickname,
        vector<std_child_info>& out_children,
        string& out_error_msg);
    // �ϱ� ����, ��ȸ �� ��� �߰� ����

private:
    unique_ptr<sql::Connection> conn_;
};
