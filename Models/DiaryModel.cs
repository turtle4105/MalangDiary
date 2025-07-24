using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.ListView;

namespace MalangDiary.Models {
    public partial class DiaryModel
    {

        /** Constructor **/
        public  DiaryModel(SocketManager socket, UserSession session) {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");

            _socket = socket;
            _session = session;
        }



        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        public string VoicePath { get; set; }
        public DiaryInfo CurrentDiaryInfo;

        private string? resultTitle;
        private string? resultText;
        private string[] resultEmotions = Array.Empty<string>();

        public string? GetResultTitle() => resultTitle;
        public string? GetResultText() => resultText;
        public string[] GetResultEmotions() => resultEmotions;


        // 임시 경로 저장 필드 추가
        private string? tempAudioFilePath;


        /** Member Methods **/

        /* Set Temporary Audio Path */
        public void SetTempAudioPath(string path) {
            tempAudioFilePath = path;
        }

        /* Get Temporary Audio Path */
        public string? GetTempAudioPath() {
            return tempAudioFilePath;
        }

        /* Set Diary Voice Path */
        public void SetDiaryVoicePath(string path) {
            VoicePath = path;
        }

        /* SendDiaryAsync */
        [RelayCommand] public async Task SendDiaryAsync()
        {
            if (string.IsNullOrEmpty(VoicePath) || !File.Exists(VoicePath))
            {
                Console.WriteLine("[DiaryModel] 음성 파일 없음: " + VoicePath);
                return;
            }

            int childUid = _session.GetCurrentChildUid();
            string fileName = Path.GetFileName(VoicePath);

            var jsonObj = new {
                PROTOCOL = "GEN_DIARY",
                //CHILD_UID = childUid,
                //FILENAME = fileName
            };


            string jsonStr = JsonConvert.SerializeObject(jsonObj);
            byte[] fileBytes = await File.ReadAllBytesAsync(VoicePath);

            WorkItem item = new WorkItem {
                json = jsonStr,
                payload = new byte[0],
                path = ""
                //payload = fileBytes,
                //path = VoicePath
            };

            _socket.Send(item);
            Console.WriteLine("[DiaryModel] 일기 생성 요청 전송 완료");
        }

        public void StartListening()
        {
            Task.Run(() =>
            {
                while (true)
                {
                    WorkItem item = _socket.Receive();

                    var json = Newtonsoft.Json.Linq.JObject.Parse(item.json);
                    string protocol = json["PROTOCOL"]?.ToString() ?? "";

                    if (protocol == "GEN_DIARY_RESULT")
                    {
                        Console.WriteLine("[DiaryModel] GEN_DIARY_RESULT 수신");

                        resultTitle = json["TITLE"]?.ToString() ?? "";
                        resultText = json["TEXT"]?.ToString() ?? "";

                        resultEmotions = json["EMOTIONS"]?
                            .Select(e => e?["EMOTION"]?.ToString() ?? "")
                            .ToArray() ?? Array.Empty<string>();

                                     // 페이지 전환
                        WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.MdfDiary));
                       
                    }
                }
            });
        }

    }
}
