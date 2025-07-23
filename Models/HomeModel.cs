using MalangDiary.Helpers;
using MalangDiary.Models;
using MalangDiary.Services;
using MalangDiary.Structs;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using static System.Net.WebRequestMethods;
using System.Threading;
using File = System.IO.File;

namespace MalangDiary.Models {
    public class HomeModel {
        public HomeModel(SocketManager socket, UserSession session) {
            // 생성자에서 필요한 초기화 작업을 수행할 수 있습니다.
            Console.WriteLine("[HomeModel] HomeModel 인스턴스가 생성되었습니다.");

            _socket = socket;
            _session = session;

        }

        /* Member Variables */
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        private DiaryInfo LatestDiary;      // DiaryInfo of today's diary


        /* Member Mehtods */
        public void LoadData() {
            GetLatestDiary();
        }

        public DiaryInfo GetDiaryInfo() {
            return LatestDiary;
        }

        public DiaryInfo GetLatestDiary() {

            DiaryInfo ResultDiaryInfo = new();
            ResultDiaryInfo.Uid = 0;    // Default uid = 0 (No Diary)

            if (_session.GetCurrentChildUid() == 0) {
                Console.WriteLine("[HomeModel] 현재 선택된 아기가 없습니다.");
                return ResultDiaryInfo;
            }

            // 현재 선택된 아기의 UID를 가져옵니다.
            int CurrentChildUid = _session.GetCurrentChildUid();

            // json 생성
            JObject jsonData = new()
            {
                {"PROTOCOL", "GET_LATEST_DIARY" },
                {"CHILD_UID", CurrentChildUid }
            };

            // 보내는 메세지작업
            WorkItem sendingItem = new WorkItem
            {
                json = jsonData.ToString(),
                payload = [],
                path = ""
            };

            _socket.Send(sendingItem);

            // 받는 작업
            WorkItem recvItem = new();
            recvItem = _socket.Receive();

            jsonData = JObject.Parse(recvItem.json);
            byte[] byteData = recvItem.payload;
            string filePath = recvItem.path;

            string protocol = jsonData["PROTOCOL"]!.ToString();
            string response = jsonData["RESP"]!.ToString();   // not null

            if (protocol == "GET_LATEST_DIARY" && response == "SUCCESS") {

                ResultDiaryInfo.Uid = jsonData["DIARY_UID"]!.ToObject<int>();
                ResultDiaryInfo.IntWeather = jsonData["WEATHER"]!.ToObject<int>();
                ResultDiaryInfo.Weather = WeatherConveter.ConvertWeatherCodeToText(ResultDiaryInfo.IntWeather);
                ResultDiaryInfo.Date = jsonData["CREATE_AT"]!.ToString();
                ResultDiaryInfo.Emotions = new();   // List<string> 초기화 필요
                if (jsonData["EMOTIONS"] is not null) {
                    JArray? ArrEmotions = jsonData["EMOTIONS"]!.ToObject<JArray>();

                    // JArray의 열거자(반복가능객체) 생성
                    var iterator = ArrEmotions?.GetEnumerator();

                    while (iterator.MoveNext()) {
                        var emotion = iterator.Current as JObject;
                        if (emotion is not null) {
                            Console.WriteLine("emotion[\"EMOTION\"]!.ToString():" + emotion["EMOTION"]!.ToString());
                            ResultDiaryInfo.Emotions!.Add(emotion["EMOTION"]!.ToString());
                        }
                    }
                }
                LatestDiary = ResultDiaryInfo;
                _session.SetCurrentDiaryUid(ResultDiaryInfo.Uid);

                string FixedImgPath = "./Images/TodaysDiaryImg.jpg";
                WriteToFile(FixedImgPath, byteData);
            }
            else if ( protocol == "GET_LATEST_DIARY" && response == "FAIL" ) {
                ResultDiaryInfo.Uid = 0;
            }
                return ResultDiaryInfo;
        }


        public bool CheckImgDirExists(DirectoryInfo dir) {
            if( dir.Exists is false ) {     // No Dir -> Create Dir -> return true
                dir.Create(); 
                return true;
            }
            else if( dir.Exists is true ) { // Yes Dir -> return true
                return true;
            }
            else {
                Console.WriteLine("Failed to Check Dir");
                return false;               // Exception -> return false
            }
        }


        public bool WriteToFile(string filePath, byte[] byteArray) {
            string dirPath = "./Images";
            DirectoryInfo dir = new DirectoryInfo(dirPath);
            bool dirCheck = CheckImgDirExists(dir);

            if (!dirCheck) {
                Console.WriteLine("Failed to create Dir");
                return false;
            }

            try {
                File.WriteAllBytes(filePath, byteArray); // 파일이 없으면 자동 생성
                Console.WriteLine($"Image has been saved into {filePath}");
                return true;
            }
            catch (IOException ex) {
                Console.WriteLine("파일 저장 중 오류: " + ex.Message);
                return false;
            }
        }
    }
}
