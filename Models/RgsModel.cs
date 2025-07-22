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
        public RgsModel(SocketManager socket, UserSession session)
        {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
        }

        private readonly SocketManager _socket;
        private readonly UserSession _session;
        public bool IsVoiceSet { get; private set; } = false;

        public (bool isSuccess, string message) RegisterChild(string name, string birthdate, string gender, string iconColor)
        {
            int parentUid = _session.GetCurrentParentUid();

            if (string.IsNullOrWhiteSpace(name) ||
                string.IsNullOrWhiteSpace(birthdate) ||
                string.IsNullOrWhiteSpace(gender) ||
                string.IsNullOrWhiteSpace(iconColor))
            {
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

            string json = JsonConvert.SerializeObject(jsonData);
            WorkItem sendItem = new()
            {
                json = json,
                payload = new byte[0],
                path = string.Empty
            };

            _socket.Send(sendItem);
            WorkItem response = _socket.Receive();

            JObject resJson = JObject.Parse(response.json);
            string protocol = resJson["PROTOCOL"]?.ToString() ?? "";
            string resp = resJson["RESP"]?.ToString() ?? "";
            string message = resJson["MESSAGE"]?.ToString() ?? "";

            if (protocol == "REGISTER_CHILD" && resp == "SUCCESS")
            {
                int childUid = resJson["CHILD_UID"]?.ToObject<int>() ?? -1;
                Console.WriteLine($"[RgsModel] 자녀 등록 성공: UID={childUid}");
                return (true, message);
            }
            else
            {
                Console.WriteLine($"[RgsModel] 자녀 등록 실패: {message}");
                return (false, message);
            }
        }
        public (bool isSuccess, string message) SetBabyVoice(string filePath)
        {
            int childUid = 2; // ← 실제 자녀 UID로 교체 필요
            string fileName = $"{childUid}_setvoice.wav";

            JObject json = new JObject {
        { "PROTOCOL", "SETTING_VOICE" },
        { "CHILD_UID", childUid },
        { "FILENAME", fileName }
    };

            WorkItem item = new WorkItem
            {
                json = json.ToString(),
                payload = File.ReadAllBytes(filePath),
                path = filePath
            };

            _socket.Send(item);
            Console.WriteLine("[SetBabyVoice] 전송 완료");

            // 🟡 여기서 서버 응답 수신
            WorkItem response = _socket.Receive();
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
