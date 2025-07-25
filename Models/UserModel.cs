using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace MalangDiary.Models
{
    public class UserModel
    {
        /*** Member Variables ***/

        private void PrintHex(string title, byte[] bytes)
        {
            Console.WriteLine($"{title} ({bytes.Length} bytes):");
            Console.WriteLine(BitConverter.ToString(bytes).Replace("-", " "));
        }

        // 외부 서비스 및 세션 정보
        private readonly SocketManager _socket;
        private readonly UserSession _session;

        // 부모 및 자녀 정보
        private ParentInfo ParentInfo;                    // 부모 정보
        public List<ChildInfo> ChildrenInfo { get; set; } = new();
        private ChildInfo SelectedChildInfo;              // 선택된 자녀 정보

        /*** Constructor ***/
        public UserModel(SocketManager socket, UserSession session)
        {
            Console.WriteLine("[UserModel] UserModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
        }

        /*** ChildInfo 관련 메서드 ***/


        /** Member Methods **/
        
        /* SetAllChildInfo */
        public void SetAllChildInfo(List<ChildInfo> childInfos) {
            ChildrenInfo = childInfos;
        }

        public void AddChildInfo(ChildInfo childInfo)
        {
            ChildrenInfo.Add(childInfo);
        }

        public List<ChildInfo> GetAllChildInfo()
        {
            return ChildrenInfo;
        }

        public void SetSelectedChildInfo(ChildInfo childInfo)
        {
            SelectedChildInfo = childInfo;
        }

        public ChildInfo GetSelectedChildInfo()
        {
            return SelectedChildInfo;
        }

        /*** 회원가입 요청 (REGISTER_PARENTS) ***/
        public (bool isSuccess, string message) RegisterParents(string name, string email, string password, string phone)
        {
            if (string.IsNullOrWhiteSpace(name) || string.IsNullOrWhiteSpace(email) ||
                string.IsNullOrWhiteSpace(password) || string.IsNullOrWhiteSpace(phone))
            {
                return (false, "입력값이 누락되었습니다.");
            }

            if (phone.Length != 11 || !phone.All(char.IsDigit))
            {
                return (false, "전화번호 형식이 올바르지 않습니다.");
            }

            string fixedPhone = phone.Insert(3, "-").Insert(8, "-"); // 010-1234-5678

            var jsonData = new JObject
            {
                { "PROTOCOL", "REGISTER_PARENTS" },
                { "ID", email },
                { "PW", password },
                { "NICKNAME", name },
                { "PHONE_NUMBER", fixedPhone }
            };

            var send = new WorkItem
            {
                json = JsonConvert.SerializeObject(jsonData),
                payload = [],
                path = ""
            };

            _socket.Send(send);

            WorkItem response = _socket.Receive();
            JObject responseJson = JObject.Parse(response.json);

            string protocol = responseJson["PROTOCOL"]?.ToString() ?? "";
            if (protocol != "REGISTER_PARENTS") return (false, "서버 응답이 올바르지 않습니다.");

            string resp = responseJson["RESP"]?.ToString() ?? "";
            string message = responseJson["MESSAGE"]?.ToString() ?? "";

            if (resp == "SUCCESS")
            {
                ParentInfo = new ParentInfo
                {
                    Uid = responseJson["PARENTS_UID"]?.ToObject<int>() ?? -1
                };
                return (true, message);
            }
            else
            {
                return (false, message);
            }
        }


        /* Method for Protocol-LogIn */
        public /*async Task<*/bool/*>*/ LogIn(string email, string password) {

            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "LOGIN" },
                { "ID", email },
                { "PW", password }
            };

            var sendItem = new WorkItem
            {
                json = JsonConvert.SerializeObject(jsonData),
                payload = [],
                path = ""
            };

            _socket.Send(sendItem);

            WorkItem response = _socket.Receive();

            jsonData = JObject.Parse(response.json);
            string protocol = jsonData["PROTOCOL"]?.ToString() ?? "";
            string result = jsonData["RESP"]?.ToString() ?? "";

            if (protocol == "LOGIN" && result == "SUCCESS")
            {
                ParentInfo parentInfo = new()
                {
                    Name = jsonData["NICKNAME"]?.ToString() ?? "",
                    Uid = jsonData["PARENTS_UID"]?.ToObject<int>() ?? -1
                };

                JArray childrenArray = (JArray)jsonData["CHILDREN"]!;
                foreach (JObject child in childrenArray.Cast<JObject>())
                {
                    string birth = (string)child["BIRTH"]!;
                    if (string.IsNullOrWhiteSpace(birth)) birth = "00000000";

                    ChildInfo childInfo = new()
                    {
                        Uid = (int)child["CHILDUID"]!,
                        Name = (string)child["NAME"]!,
                        BirthDate = birth,
                        Age = DateTime.Now.Year - int.Parse(birth[..4]),
                        Gender = (string)child["GENDER"]!,
                        IconColor = (string)child["ICON_COLOR"]!
                    };

                    AddChildInfo(childInfo);
                }

                _session.SetCurrentParentUid(parentInfo.Uid);
                _session.SetCurrentChildUid(ChildrenInfo[0].Uid);

                Console.WriteLine($"[UserModel] LogIn Completed");
                return true;
            }
            else if (protocol == "LOGIN" && result == "FAIL") {
                return false;
            }
            else {
                Console.WriteLine($"[UserModel] 로그인 응답 오류: {result}");
                return false;
            }
        }

        public void SetChildrenFromResponse(JObject resJson)
        {
            ChildrenInfo.Clear();

            if (resJson["CHILDREN"] is not JArray childrenArray)
            {
                Console.WriteLine("[UserModel] CHILDREN 배열이 존재하지 않음");
                return;
            }

            foreach (JObject child in childrenArray)
            {
                string birth = (string)child["BIRTH"]!;
                if (string.IsNullOrWhiteSpace(birth)) birth = "00000000";

                ChildInfo info = new()
                {
                    Uid = (int)child["CHILDUID"]!,
                    Name = (string)child["NAME"]!,
                    BirthDate = birth,
                    Age = DateTime.Now.Year - int.Parse(birth[..4]),
                    Gender = (string)child["GENDER"]!,
                    IconColor = (string)child["ICON_COLOR"]!
                };

                ChildrenInfo.Add(info);
            }

            Console.WriteLine($"[UserModel] 자녀 {ChildrenInfo.Count}명 저장됨");
        }

    }
}
