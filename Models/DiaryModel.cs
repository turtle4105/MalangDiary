using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Windows;
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


        public DiaryModel(SocketManager socket, UserSession session, UserModel userModel)
        {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");

            _socket = socket;
            _session = session;
            _userModel = userModel;
        }

        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        private readonly UserModel _userModel;
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
        [RelayCommand] public void SendDiaryAsync()
        {
            if (string.IsNullOrEmpty(VoicePath) || !File.Exists(VoicePath))
            {
                Console.WriteLine("[DiaryModel] 음성 파일 없음: " + VoicePath);
                return;
            }

            int childUid = _session.GetCurrentChildUid();

            // UserModel에서 자녀 이름 찾기
            string childName = _userModel.GetAllChildInfo()
                .FirstOrDefault(c => c.Uid == childUid).Name ?? "Unknown";

            var jsonObj = new
            {
                PROTOCOL = "GEN_DIARY",
                CHILD_UID = childUid,
                NAME = childName
            };

            string jsonStr = JsonConvert.SerializeObject(jsonObj);
            byte[] fileBytes =  File.ReadAllBytes(VoicePath);

            WorkItem item = new WorkItem {
                json = jsonStr,
                payload = fileBytes,
                path = VoicePath
            };

            _socket.Send(item);
            Console.WriteLine("[DiaryModel] 일기 생성 요청 전송 완료");
        }

        public void StartListening()
        {
            Console.WriteLine("[DiaryModel] StartListeningOnce() 호출됨");

            try
            {
                Console.WriteLine("[DiaryModel] 서버 응답 대기 중...");
                WorkItem item = _socket.Receive(); // 여기서 대기

                Console.WriteLine("[DiaryModel] 서버 응답 수신: " + item.json);

                var json = JObject.Parse(item.json);
                string protocol = json["PROTOCOL"]?.ToString() ?? "";

                if (protocol == "GEN_DIARY_RESULT")
                {
                    Console.WriteLine("[DiaryModel] GEN_DIARY_RESULT 수신");

                    resultTitle = json["TITLE"]?.ToString() ?? "";
                    resultText = json["TEXT"]?.ToString() ?? "";

                    resultEmotions = json["EMOTIONS"]?
                        .Select(e => e?["EMOTION"]?.ToString() ?? "")
                        .ToArray() ?? Array.Empty<string>();

                    CurrentDiaryInfo = new DiaryInfo
                    {
                        Uid = json["DIARY_UID"]?.ToObject<int>() ?? 0,
                        Title = resultTitle ?? "",
                        Text = resultText ?? "",
                        Emotions = resultEmotions.ToList(),
                        Date = json["CREATE_AT"]?.ToString() ?? "",
                        IntWeather = json["WEATHER"]?.ToObject<int>() ?? 0,
                        Weather = json["WEATHER_STR"]?.ToString() ?? "",
                        IsLiked = json["IS_LIKED"]?.ToObject<bool>() ?? false,
                        PhotoFileName = json["PHOTO_PATH"]?.ToString() ?? ""
                    };

                    Application.Current.Dispatcher.Invoke(() =>
                    {
                        Console.WriteLine("[DiaryModel] MdfDiary로 이동 메시지 전송");
                        WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.MdfDiary));
                    });
                }
                else
                {
                    Console.WriteLine($"[DiaryModel] 예상하지 못한 프로토콜: {protocol}");
                }
               

            }
            catch (Exception ex)
            {
                Console.WriteLine("[StartListeningOnce 예외] " + ex.Message);
            }
        }

    }
}
