using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Models {
    public class DiaryModel {
        public DiaryModel(SocketManager socket, UserSession session) {
            // 생성자에서 필요한 초기화 작업을 수행할 수 있습니다.
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");

            _socket = socket;
            _session = session;
        }

        private readonly SocketManager _socket;
        private readonly UserSession _session;

        // 임시 경로 저장 필드 추가
        private string? tempAudioFilePath;

        public void SetTempAudioPath(string path)
        {
            tempAudioFilePath = path;
        }

        public string? GetTempAudioPath()
        {
            return tempAudioFilePath;
        }

        public string VoicePath { get; set; }
        public void SetDiaryVoicePath(string path)
        {
            VoicePath = path;
        }

        public async Task SendDiaryAsync()
        {
            if (string.IsNullOrEmpty(VoicePath) || !File.Exists(VoicePath))
            {
                Console.WriteLine("[DiaryModel] 음성 파일 없음: " + VoicePath);
                return;
            }

            int childUid = _session.GetCurrentChildUid();
            string fileName = $"{childUid}_diary.wav";  // or Path.GetFileName(VoicePath)

            var jsonObj = new
            {
                PROTOCOL = "GEN_DIARY",
                CHILD_UID = childUid,
                FILENAME = fileName
            };


            string jsonStr = JsonConvert.SerializeObject(jsonObj);
            byte[] fileBytes = await File.ReadAllBytesAsync(VoicePath);

            WorkItem item = new WorkItem
            {
                json = jsonStr,
                payload = fileBytes,
                path = VoicePath
            };

            _socket.Send(item);
            Console.WriteLine("[DiaryModel] 일기 생성 요청 전송 완료");
        }
    }
}
