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
        public RgsModel(SocketManager socket, UserSession session) {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
        }



        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        public bool IsVoiceSet { get; private set; } = false;

        //public (bool isSuccess, string message) RegisterChild(string name, string birthdate, string gender, string iconColor)
        //{
        //    int parentUid = _session.GetCurrentParentUid();
        //    //int parentUid = 38;

        //    if (string.IsNullOrWhiteSpace(name) ||
        //        string.IsNullOrWhiteSpace(birthdate) ||
        //        string.IsNullOrWhiteSpace(gender) ||
        //        string.IsNullOrWhiteSpace(iconColor))
        //    {
        //        return (false, "입력값이 누락되었습니다.");
        //    }

        //    JObject jsonData = new()
        //    {
        //        { "PROTOCOL", "REGISTER_CHILD" },
        //        { "PARENTS_UID", parentUid },
        //        { "NAME", name },
        //        { "BIRTHDATE", birthdate },
        //        { "GENDER", gender },
        //        { "ICON_COLOR", iconColor }
        //    };

        //    string json = JsonConvert.SerializeObject(jsonData);
        //    WorkItem sendItem = new()
        //    {
        //        json = json,
        //        payload = new byte[0],
        //        path = string.Empty
        //    };

        //    _socket.Send(sendItem);
        //    WorkItem response = _socket.Receive();

        //    JObject resJson = JObject.Parse(response.json);
        //    string protocol = resJson["PROTOCOL"]?.ToString() ?? "";
        //    string resp = resJson["RESP"]?.ToString() ?? "";
        //    string message = resJson["MESSAGE"]?.ToString() ?? "";

        //    if (protocol == "REGISTER_CHILD" && resp == "SUCCESS")
        //    {
        //        int childUid = resJson["CHILD_UID"]?.ToObject<int>() ?? -1;
        //        Console.WriteLine($"[RgsModel] 자녀 등록 성공: UID={childUid}");
        //        return (true, message);
        //    }
        //    else
        //    {
        //        Console.WriteLine($"[RgsModel] 자녀 등록 실패: {message}");
        //        return (false, message);
        //    }
        //}

        public (bool isSuccess, string message) RegisterChild(string name, string birthdate, string gender, string iconColor)
        {
            int parentUid = _session.GetCurrentParentUid();

            Console.WriteLine("▶ [RegisterChild] 호출됨");
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
                Console.WriteLine("❌ 입력값 누락");
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
            Console.WriteLine("📤 보낼 JSON:");
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
                Console.WriteLine("✅ 요청 전송 완료");

                WorkItem response = _socket.Receive();
                Console.WriteLine("📥 서버 응답 수신 완료");

                Console.WriteLine("📄 응답 JSON:");
                Console.WriteLine(response.json);

                JObject resJson = JObject.Parse(response.json);
                string protocol = resJson["PROTOCOL"]?.ToString() ?? "";
                string resp = resJson["RESP"]?.ToString() ?? "";
                string message = resJson["MESSAGE"]?.ToString() ?? "";

                if (protocol == "REGISTER_CHILD" && resp == "SUCCESS")
                {
                    int childUid = resJson["CHILD_UID"]?.ToObject<int>() ?? -1;
                    Console.WriteLine($"✅ 자녀 등록 성공: UID = {childUid}");
                    _session.SetCurrentChildUid(childUid);  // 💡 꼭 UID 저장
                    return (true, message);
                }
                else
                {
                    Console.WriteLine($"❌ 자녀 등록 실패: {message}");
                    return (false, message);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"🔥 예외 발생: {ex.Message}");
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
