#pragma once
#include <nlohmann/json.hpp>
class DBManager;

class ProtocolHandler {
public:
    //ȸ������
    static nlohmann::json handle_RegisterParents(const nlohmann::json& json, DBManager& db);
    //�α���
    static nlohmann::json handle_Login(const nlohmann::json& json, DBManager& db);
    //���̵��
    //static nlohmann::json handle_RegisterChild(const nlohmann::json& json, DBManager& db);

};

/*������� ���� �ľ� �� json �Ľ� �� �������� �б� �� DBManager�� ���� ����
�� ����� ProtocolHandler�� ���� �� ���� JSON ���� �� send�� Ŭ���̾�Ʈ�� ��ȯ��*/