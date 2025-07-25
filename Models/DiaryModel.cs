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
        public DiaryModel(SocketManager ?socket, UserSession ?session, UserModel ?userModel)
        {
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");

            _socket    = socket    !;
            _session   = session   !;
            _userModel = userModel !;
        }

        public void SendModifyDiary(WorkItem item)
        {
            Console.WriteLine("[DiaryModel] MODIFY_DIARY 전송 중...");
            _socket.Send(item);
        }

        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        private readonly UserModel _userModel;
        public string ? VoicePath { get; set; }
        public DiaryInfo? CurrentDiaryInfo;

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
            Console.WriteLine("[DiaryModel] StartListening() 호출됨");
            Task.Run(() =>
            {
                while (true)
                {
                    try
                    {
                        Console.WriteLine("[DiaryModel] 서버 응답 대기 중...");
                        WorkItem item = _socket.Receive();
                        Console.WriteLine("[DiaryModel] 서버 응답 수신: " + item.json);

                        var json = JObject.Parse(item.json);
                        string protocol = json["PROTOCOL"]?.ToString() ?? "";

                        if (protocol == "GEN_DIARY_RESULT")
                        {
                            Console.WriteLine("[DiaryModel] GEN_DIARY_RESULT 수신");

                            // 값 추출
                            int diaryUid = json["DIARY_UID"]?.ToObject<int>() ?? 0;
                            string title = json["TITLE"]?.ToString() ?? "(제목 없음)";
                            string date = json["DATE"]?.ToString() ?? string.Empty;
                            string text = json["TEXT"]?.ToString() ?? json["MESSAGE"]?.ToString() ?? "(내용 없음)";
                            string[] emotions;

                            var emotionsToken = json["EMOTIONS"];
                            if (emotionsToken != null && emotionsToken.Type == JTokenType.String)
                            {
                                // 서버에서 문자열로 온 경우
                                emotions = emotionsToken.ToString()
                                    .Split(',', StringSplitOptions.RemoveEmptyEntries)
                                    .Select(e => e.Trim())
                                    .ToArray();
                            }
                            else if (emotionsToken != null && emotionsToken.Type == JTokenType.Array)
                            {
                                // JSON 배열로 온 경우
                                emotions = emotionsToken
                                    .Select(e => e?["EMOTION"]?.ToString() ?? "")
                                    .ToArray();
                            }
                            else
                            {
                                emotions = Array.Empty<string>();
                            }

                            string weatherText = json["WEATHER"]?.ToString() ?? "알 수 없음";
                            // CurrentDiaryInfo 설정
                            CurrentDiaryInfo = new DiaryInfo
                            {
                                Uid = diaryUid,
                                Title = title,
                                Text = text,
                                Emotions = emotions.ToList(),
                                Date = date,
                                Weather = weatherText,
                                IsLiked = false,
                                PhotoFileName = ""
                            };

                            // 결과 저장
                            resultTitle = title;
                            resultText = text;
                            resultEmotions = emotions;

                            _session.SetCurrentDiaryUid(diaryUid);

                            // 페이지 전환
                            Application.Current.Dispatcher.Invoke(() =>
                            {
                                Console.WriteLine("[DiaryModel] MdfDiary로 이동 메시지 전송");
                                WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.MdfDiary));
                            });
                            break; 
                        }
                        else if (protocol == "MODIFY_DIARY")
                        {
                            string resp = json["RESP"]?.ToString() ?? "";
                            string message = json["MESSAGE"]?.ToString() ?? "";

                            Console.WriteLine($"[DiaryModel] MODIFY_DIARY 응답 수신: {resp} - {message}");

                            if (resp == "SUCCESS")
                            {
                                // 홈 화면으로 전환
                                Application.Current.Dispatcher.Invoke(() =>
                                {
                                    Console.WriteLine("[DiaryModel] 일기 수정 성공 → 홈 화면으로 이동");
                                    WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
                                });
                            }
                            else
                            {
                                Console.WriteLine("[DiaryModel] 일기 수정 실패: " + message);
                            }
                            break; // 응답 처리 완료 후 루프 탈출
                        }
                        else
                        {
                            Console.WriteLine($"[DiaryModel] 예상하지 못한 프로토콜 수신: {protocol}");
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("[StartListening 예외] " + ex.Message);
                        break;
                    }
                }
            });
        }
    }
}
