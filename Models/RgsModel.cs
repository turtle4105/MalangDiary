using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.IO;


namespace MalangDiary.Models
{
    public class RgsModel
    {
        /** Constructer **/
        public RgsModel(SocketManager socket, UserSession session, UserModel userModel) {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
            _userModel = userModel;
        }

        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        public bool IsVoiceSet { get; private set; } = false;
        private readonly UserModel _userModel;


        public (bool isSuccess, string message) RegisterChild(string name, string birthdate, string gender, string iconColor)
        {
            int parentUid = _session.GetCurrentParentUid();

            Console.WriteLine(" [RegisterChild] 호출됨");
            Console.WriteLine($" - name: {name}");
            Console.WriteLine($" - birthdate: {birthdate}");
            Console.WriteLine($" - gender: {gender}");
            Console.WriteLine($" - iconColor: {iconColor}");
            Console.WriteLine($" - parentUid: {parentUid}");

            if (string.IsNullOrWhiteSpace(name) ||
                string.IsNullOrWhiteSpace(birthdate) ||
                string.IsNullOrWhiteSpace(gender) ||
                string.IsNullOrWhiteSpace(iconColor))
            {
                Console.WriteLine(" 입력값 누락");
                return (false, "입력값이 누락되었습니다.");
            }

            JObject jsonData = new()
            {
                { "PROTOCOL", "REGISTER_CHILD" },
                { "PARENTS_UID", parentUid },
                { "NAME", name },
                { "BIRTHDATE", birthdate },
                { "GENDER", gender },
                { "ICON_COLOR", iconColor }
            };

            string json = JsonConvert.SerializeObject(jsonData, Formatting.Indented);
            Console.WriteLine(" 보낼 JSON:");
            Console.WriteLine(json);

            WorkItem sendItem = new()
            {
                json = json,
                payload = new byte[0],
                path = string.Empty
            };

            try
            {
                _socket.Send(sendItem);
                Console.WriteLine(" 요청 전송 완료");

                WorkItem response = _socket.Receive();
                Console.WriteLine("서버 응답 수신 완료");

                Console.WriteLine(" 응답 JSON:");
                Console.WriteLine(response.json);

                JObject resJson = JObject.Parse(response.json);
                string protocol = resJson["PROTOCOL"]?.ToString() ?? "";
                string resp = resJson["RESP"]?.ToString() ?? "";
                string message = resJson["MESSAGE"]?.ToString() ?? "";
                int new_child = resJson["NEW_CHILD"]?.ToObject<int>() ?? -1;
                _session.SetCurrentChildUid(new_child);

                if (protocol == "REGISTER_CHILD" && resp == "SUCCESS")
                {
                    int childUid = resJson["CHILD_UID"]?.ToObject<int>() ?? -1;
                    _userModel.SetChildrenFromResponse(resJson);

                     Console.WriteLine("[DEBUG] REGISTER_CHILD 응답 전체:\n" + resJson.ToString());

                    // 방어 코드 추가
                    var allChildren = _userModel.GetAllChildInfo();
                    if (allChildren.Count == 0)
                    {
                        Console.WriteLine("[RgsChdViewModel] 자녀 등록 후 ChildrenInfo 비어 있음");
                        return (false, "자녀 정보가 로드되지 않았습니다.");
                    }

                    _session.SetCurrentChildIndex(0);
                    _session.SetCurrentChildUid(allChildren[0].Uid);

                    return (true, message);
                }

                else
                {
                    Console.WriteLine($" 자녀 등록 실패: {message}");
                    return (false, message);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($" 예외 발생: {ex.Message}");
                return (false, "서버 응답 중 오류 발생");
            }
        }

        public (bool isSuccess, string message) SetBabyVoice(string filePath)
        {
            int childUid = _session.GetCurrentChildUid();
            string fileName = $"{childUid}_setvoice.wav";

            Console.WriteLine($"[SetBabyVoice] childUid: {childUid}");
            Console.WriteLine($"[SetBabyVoice] filePath: {filePath}");

            if (!File.Exists(filePath))
            {
                Console.WriteLine("[SetBabyVoice] 파일이 존재하지 않습니다.");
                return (false, "녹음 파일 없음");
            }

            byte[] payload = File.ReadAllBytes(filePath);
            Console.WriteLine($"[SetBabyVoice] 파일 크기: {payload.Length} bytes");

            JObject json = new JObject {
                { "PROTOCOL", "SETTING_VOICE" },
                { "CHILD_UID", childUid },
                { "FILENAME", fileName }
            };

            Console.WriteLine($"[SetBabyVoice] 보낼 JSON: {json}");

            WorkItem item = new WorkItem
            {
                json = json.ToString(),
                payload = payload,
                path = filePath
            };

            _socket.Send(item);
            Console.WriteLine("[SetBabyVoice] 전송 완료");

            WorkItem response = _socket.Receive();
            Console.WriteLine($"[SetBabyVoice] 서버 응답 JSON: {response.json}");

            JObject resJson = JObject.Parse(response.json);

            string protocol = resJson["PROTOCOL"]?.ToString() ?? "";
            string resp = resJson["RESP"]?.ToString() ?? "";
            string message = resJson["MESSAGE"]?.ToString() ?? "";

            if (protocol == "SETTING_VOICE" && resp == "SUCCESS")
            {
                Console.WriteLine("[SetBabyVoice] 등록 성공");
                IsVoiceSet = true;
                return (true, message);
            }
            else
            {
                Console.WriteLine($"[SetBabyVoice] 등록 실패: {message}");
                return (false, message);
            }
        }


    }
}
