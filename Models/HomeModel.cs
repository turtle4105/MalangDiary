using MalangDiary.Services;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using MalangDiary.Structs;
using MalangDiary.Models;
using MalangDiary.Helpers;

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

            // 받는 작업( 임시 )
            jsonData = JObject.Parse(_socket.Receive().json);

            string protocol = jsonData["PROTOCOL"]!.ToString();
            string response = jsonData["RESP"]!.ToString();   // null 아님

            if (protocol == "GET_LATEST_DIARY" && response == "SUCCESS") {

                ResultDiaryInfo.Uid = jsonData["DIARY_UID"]!.ToObject<int>();
                ResultDiaryInfo.IntWeather = jsonData["WEATHER"]!.ToObject<int>();
                ResultDiaryInfo.Weather = WeatherConveter.ConvertWeatherCodeToText(ResultDiaryInfo.IntWeather);
                ResultDiaryInfo.Date = jsonData["CREATE_AT"]!.ToString();
                if (jsonData["EMOTIONS"] is not null) {
                    JArray? ArrEmotions = jsonData["EMOTIONS"]!.ToObject<JArray>();

                    // JArray의 열거자(반복가능객체) 생성
                    var iterator = ArrEmotions?.GetEnumerator();

                    while (iterator.MoveNext()) {
                        var emotion = iterator.Current as JObject;
                        if (emotion is not null) {
                            ResultDiaryInfo.Emotions!.Append(emotion["EMOTION"]!.ToString());
                        }
                    }
                }
                LatestDiary = ResultDiaryInfo;
                _session.SetCurrentDiaryUid(ResultDiaryInfo.Uid);
            }
            else if ( protocol == "GET_LATEST_DIARY" && response == "FAIL" ) {
                ResultDiaryInfo.Uid = 0;
            }
                return ResultDiaryInfo;
        }
    }
}
