using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Models {
    /// <summary>
    /// [Variables]: 
    /// 사용자 정보
    /// [Methods]:
    /// </summary>
    /// <param name="socket"></param>
    /// <param name="session"></param>
    public class UserModel {

        /// <summary>
        /// '생성자: UserModel'
        /// </summary>
        /// <param name="socket"></param>
        /// <param name="session"></param>
        public UserModel(SocketManager socket, UserSession session) {
            // 생성자의 Parameter로 SocketManager와 UserSession을 주입
            Console.WriteLine("[UserModel] UserModel 인스턴스가 생성되었습니다.");

            // Parameter로 받은 SocketManager와 UserSession을 필드에 저장
            _socket = socket;
            _session = session;
        }


        /* Member Variables */
        private readonly SocketManager _socket;
        private readonly UserSession _session;

        Structs.ParentInfo ParentInfo;           // 부모 정보
        Structs.ChildInfo[] AllChildInfo = [];   // 전체 아기 정보
        Structs.ChildInfo SelectedChildInfo;     // 선택된 아기 정보


        /* Member Methods */
        public void SetAllChildInfo(Structs.ChildInfo[] childInfos) {
            AllChildInfo = childInfos;
        }

        public Structs.ChildInfo[] GetAllChildInfo() {
            return AllChildInfo;
        }

        public void SetSelectedChildInfo(Structs.ChildInfo childInfo) {
            SelectedChildInfo = childInfo;
        }

        public Structs.ChildInfo GetSelectedChildInfo() {
            return SelectedChildInfo;
        }


        public (bool isSuccess, string message) RegisterParents(string name, string email, string password, string phone) {

            if (string.IsNullOrWhiteSpace(name) ||
               string.IsNullOrWhiteSpace(email) ||
               string.IsNullOrWhiteSpace(password) ||
               string.IsNullOrWhiteSpace(phone)) {
                return (false, "입력값이 누락되었습니다.");
            }

            if (phone.Length != 11 || !phone.All(char.IsDigit)) {
                return (false, "전화번호 형식이 올바르지 않습니다.");
            }

            string fixed_phone = phone.Insert(3, "-").Insert(8, "-"); // 010-1234-5678 형식으로 변환

            // json 생성
            JObject jsonData = new()
             {
                 {"PROTOCOL", "REGISTER_PARENTS" },
                 {"ID", email },
                 {"PW", password },
                 {"NICKNAME", name },
                 {"PHONE_NUMBER", fixed_phone }
             };

            string json = JsonConvert.SerializeObject(jsonData);

            WorkItem send = new WorkItem
            {
                json = json,
                payload = [],   //없음
                path = ""       //없음

            };

            _socket.Send(send);

            // 수신
            WorkItem response = _socket.Receive();

            // 받은 json 파싱
            JObject responseJson = JObject.Parse(response.json);
            string protocol = responseJson["PROTOCOL"]?.ToString() ?? "";

            if (protocol == "REGISTER_PARENTS") {
                ParentInfo parentInfo = new ParentInfo();
                parentInfo.Uid = responseJson["PARENTS_UID"]?.ToObject<int>() ?? -1;

                string resp = responseJson["RESP"]?.ToString() ?? "";
                string message = responseJson["MESSAGE"]?.ToString() ?? "";

                if (resp == "SUCCESS") {
                    return (true, message);
                }
                else if (resp == "FAIL") {
                    return (false, message);
                }
            }
            return (false, "서버 응답이 올바르지 않습니다.");
        }

        public /*async Task<*/bool/*>*/ LogIn(string email, string password) {
            // json 생성
            JObject jsonData = new()
            {
                { "PROTOCOL", "LOGIN" },
                { "ID", email },
                { "PW", password }
            };

            string json = JsonConvert.SerializeObject(jsonData);
            WorkItem sendItem = new ()
            {
                json = json,
                payload = new byte[0], // 파일 데이터가 없으므로 빈 배열
                path = string.Empty    // 파일 경로가 없으므로 빈 문자열
            };
           
            /* 전송 */
            _socket.Send(sendItem);

            // -recv 관련 메서드 호출( 임시 )-
            WorkItem response = /*await*/ _socket.Receive();

            // 받은 json 파싱
            jsonData = JObject.Parse(response.json);

            string protocol = jsonData["PROTOCOL"]!.ToString();
            string result = jsonData["RESP"]!.ToString();

            if (protocol == "LOGIN" && result == "SUCCESS") {    // 성공일때
                Structs.ParentInfo parent_info = new() {
                    Name = jsonData["NICKNAME"]!.ToString(),
                    Uid = jsonData["PARENTS_UID"]!.ToObject<int>()
                };
                // Structs.ChildInfo[] children = jsonData["CHILDREN"]!.ToObject<Structs.ChildInfo[]>()!;

                // ParentInfo = parent_info;   // 부모 정보 저장
                // AllChildInfo = children;    // 전체 아기 정보 저장
                // SelectedChildInfo = children.FirstOrDefault(); // 첫번째 아기를 선택

                Console.WriteLine($"[UserModel] 로그인 성공: {parent_info.Name} ({parent_info.Uid})");
                Console.WriteLine($"[UserModel] 선택된 아기: {SelectedChildInfo.Name} ({SelectedChildInfo.Uid})");
                Console.WriteLine($"[UserModel] 전체 아기 수: {AllChildInfo.Length}");
                Console.WriteLine($"[UserModel] 전체 아기 정보: {JsonConvert.SerializeObject(AllChildInfo, Formatting.Indented)}");

                _session.SetCurrentParentUid(parent_info.Uid);  // 현재 부모 UID 설정
                return true;
            }
            else if (protocol == "LOGIN" && result == "FAIL") {  // 실패일때
                return false;
            }
            else {
                Console.WriteLine($"[UserModel] 로그인 응답 오류: {result}");
                return false;
            }
        }
    }
}
