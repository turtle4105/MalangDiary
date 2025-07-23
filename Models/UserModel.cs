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
    public class UserModel {

        /** Constructor **/
        public UserModel(SocketManager socket, UserSession session) {
            // 생성자의 Parameter로 SocketManager와 UserSession을 주입
            Console.WriteLine("[UserModel] UserModel 인스턴스가 생성되었습니다.");

            // Parameter로 받은 SocketManager와 UserSession을 필드에 저장
            _socket = socket;
            _session = session;
        }



        /** Member Variables **/

        /* Services, Models */
        private readonly SocketManager _socket;
        private readonly UserSession _session;

        /* Parent, Children Informations */
        ParentInfo ParentInfo;           // 부모 정보
        public List<ChildInfo> ChildrenInfo { get; set; } = new();
        ChildInfo SelectedChildInfo;     // 선택된 아기 정보




        /** Member Methods **/
        
        /* SetAllChildInfo */
        public void SetAllChildInfo(List<ChildInfo> childInfos) {
            ChildrenInfo = childInfos;
        }

        /* AddChildInfo */
        public void AddChildInfo(ChildInfo childinfo) {
            ChildrenInfo.Add(childinfo);
        }

        /* GetAllChildInfo */
        public List<ChildInfo> GetAllChildInfo() {
            return ChildrenInfo;
        }

        /* SetSelectedChildInfo */
        public void SetSelectedChildInfo(Structs.ChildInfo childInfo) {
            SelectedChildInfo = childInfo;
        }

        /* GetSelectedChildInfo */
        public Structs.ChildInfo GetSelectedChildInfo() {
            return SelectedChildInfo;
        }

        /* Method for Protocol-SignUp */
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


        /* Method for Protocol-LogIn */
        public /*async Task<*/bool/*>*/ LogIn(string email, string password) {

            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "LOGIN" },
                { "ID", email },
                { "PW", password }
            };

            string sendingJson = JsonConvert.SerializeObject(jsonData); // change type: json -> string
            

            /* [2] Put json in WorkItem */
            WorkItem sendItem = new () {
                json = sendingJson,  // sending json
                payload = [],        // no file data
                path = ""            // no used
            };


            /* [3] Send the created WorkItem */
            _socket.Send(sendItem);


            /* [4] Create WorkItem rsponse(response) and receive data from the socket. */
            WorkItem response = /*await*/ _socket.Receive();


            /* [5] Parse the data. */
            jsonData = JObject.Parse(response.json);

            // parse the protocol and result
            string protocol = jsonData["PROTOCOL"]!.ToString();
            string result   = jsonData["RESP"]!.ToString();

            if (protocol == "LOGIN" && result == "SUCCESS") {
                
                ParentInfo parent_info = new() {
                    Name = jsonData["NICKNAME"]!.ToString(),
                    Uid  = jsonData["PARENTS_UID"]!.ToObject<int>()
                };

                JArray childrenArray = (JArray)jsonData["CHILDREN"]!;

                foreach (JObject child in childrenArray.Cast<JObject>()) {  // added cast<JObject>() to ensure child is JObject
                    int uid = (int)child["CHILDUID"]!;
                    string name = (string)child["NAME"]!;
                    string birth = (string)child["BIRTH"]!;
                    string gender = (string)child["GENDER"]!;
                    string iconColor = (string)child["ICON_COLOR"]!;

                    if( birth.Length == 0 ) {   // default birth = 0000-00-00
                        birth = "00000000";
                    }

                    ChildInfo childinfo = new() {
                        Uid = uid,
                        Name = name,
                        BirthDate = birth,
                        Age = (int)DateTime.Now.Year - Convert.ToInt32(birth[..4]), // Calculate Age using'..' which means '(0, 4)'
                        Gender = gender,
                        IconColor = iconColor
                    };

                    Console.WriteLine("Uid: " + childinfo.Uid);
                    Console.WriteLine("Name: " + childinfo.Name);
                    Console.WriteLine("Birthdate: " + childinfo.BirthDate);
                    Console.WriteLine("Age: " + childinfo.Age);
                    Console.WriteLine("Gender: " + childinfo.Gender);
                    Console.WriteLine("IconColor: " + childinfo.IconColor);

                    AddChildInfo(childinfo);
                }

                Console.WriteLine("ChildrenInfo[0].Uid: " + ChildrenInfo[0].Uid);
                _session.SetCurrentChildUid( ChildrenInfo[0].Uid );

                Console.WriteLine($"[UserModel] LogIn Completed");
                
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
