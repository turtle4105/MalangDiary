using MalangDiary.Helpers;
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
    public class ChkModel {

        /** Member Constructor **/
        public ChkModel(SocketManager socket, UserSession session) {
            // 생성자에서 필요한 초기화 작업을 수행할 수 있습니다.
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
        }



        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;



        /** Member Methods **/
        public void ReqCalendar() {
        }

        public void ReqGallery() {
        }

        /* Protocol - GET/SEND_DIARY_DETAIL */
        public DiaryInfo GetDiaryDetail(int diary_uid) {
            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "GET_DIARY_DETAIL" },
                { "DIARY_UID", diary_uid }
            };

            /* [2] Put json in WorkItem */
            var sendItem = new WorkItem
            {
                json = JsonConvert.SerializeObject(jsonData),
                payload = [],
                path = ""
            };

            /* [3] Send the created WorkItem */
            _socket.Send(sendItem);

            /* [4] Create WorkItem rsponse(response) & receive data from the socket. */
            WorkItem response = _socket.Receive();

            /* [5] Parse the data. */
            jsonData = JObject.Parse(response.json);
            string protocol = jsonData["PROTOCOL"]?.ToString() ?? "";
            string result = jsonData["RESP"]?.ToString() ?? "";

            /* Default returning struct */
            DiaryInfo ResultDiaryInfo = new()
            {
                Uid = 0,
                Title = "",
                Text = "",
                Weather = "",
                IsLiked = false,
                PhotoFileName = "",
                Date = "",
            };
            ResultDiaryInfo.Emotions = new();   // List<string> 초기화 필요

            if (protocol == "SEND_DIARY_DETAIL" && result == "SUCCESS") {

                ResultDiaryInfo.Uid = jsonData["DIARY_UID"]!.ToObject<int>();
                ResultDiaryInfo.Title = jsonData["TITLE"]!.ToString();
                ResultDiaryInfo.Text = jsonData["TEXT"]!.ToString();
                ResultDiaryInfo.IntWeather = jsonData["WEATHER"]!.ToObject<int>();
                ResultDiaryInfo.Weather = WeatherConveter.ConvertWeatherCodeToText(ResultDiaryInfo.IntWeather);
                ResultDiaryInfo.IsLiked = jsonData["IS_LIKED"]!.ToObject<Boolean>();
                ResultDiaryInfo.PhotoFileName = jsonData["PHOTO_PATH"]!.ToString();
                ResultDiaryInfo.Date = jsonData["CREATE_AT"]!.ToString();

                if (jsonData["EMOTIONS"] is not null) {
                    JArray? ArrEmotions = jsonData["EMOTIONS"]!.ToObject<JArray>();

                    // JArray의 열거자(반복가능객체) 생성
                    var iterator = ArrEmotions?.GetEnumerator();

                    while (iterator!.MoveNext()) {  // iterator is not null
                        var emotion = iterator.Current as JObject;
                        if (emotion is not null) {
                            Console.WriteLine("emotion[\"EMOTION\"]!.ToString():" + emotion["EMOTION"]!.ToString());
                            ResultDiaryInfo.Emotions!.Add(emotion["EMOTION"]!.ToString());
                        }
                    }
                }
                return ResultDiaryInfo;
            }
            else if (protocol == "LOGIN" && result == "FAIL") {
                return ResultDiaryInfo;
            }
            else {
                Console.WriteLine($"[UserModel] 로그인 응답 오류: {result}");
                return ResultDiaryInfo;
            }
        }


        public void DeleteDiary() {
        }

        public void UpdateDiaryLike() {
        }
    }
}
