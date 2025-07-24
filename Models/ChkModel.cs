using MalangDiary.Helpers;
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

namespace MalangDiary.Models {
    public class ChkModel {

        /** Member Constructor **/
        public ChkModel(SocketManager socket, UserSession session, BaseModel baseModel) {
            // 생성자에서 필요한 초기화 작업을 수행할 수 있습니다.
            Console.WriteLine("[RgsModel] RgsModel 인스턴스가 생성되었습니다.");
            _socket = socket;
            _session = session;
            _base = baseModel;
        }



        /** Member Variables **/
        private readonly SocketManager _socket;
        private readonly UserSession _session;
        private readonly BaseModel _base;


        /** Member Methods **/
        public void ReqCalendar() {
        }

        public void ReqGallery() {
        }

        /* Protocol - GET/SEND_DIARY_DETAIL */
        public (DiaryInfo, bool) GetDiaryDetail(int diary_uid) {
            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "GET_DIARY_DETAIL" },
                { "DIARY_UID", diary_uid }
            };
            byte[] byteData;

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
            byteData = response.payload;

            /* Parsing json */
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
                /* Save Audio File  */
                string FixedImgPath = "./Audio/DiaryAudio.wav";        // set file path
                _base.WriteWavToFile(FixedImgPath, byteData);          // save wav file

                /* Save Image File */
                bool ChkImg = true;
                if( ChkImg is true ) {
                    return (ResultDiaryInfo, ChkImg);
                }
                else if( ChkImg is false ) {
                    return (ResultDiaryInfo, ChkImg);
                }
            }
            else if (protocol == "LOGIN" && result == "FAIL") {
                return (ResultDiaryInfo, false);
            }
            else {
                Console.WriteLine($"[UserModel] 로그인 응답 오류: {result}");
                return (ResultDiaryInfo, false);
            }
            return (ResultDiaryInfo, false);
        }


        /* Get Diary Image */
        public bool GetDiaryImage() {
            WorkItem RecvItem = _socket.Receive();

            byte[] byteData = RecvItem.payload;

            string FixedFilePath = "./Images/ChkDiaryImage.jpg";

            bool ChkSaved = _base.WriteJpgToFile(FixedFilePath, byteData);

            if (ChkSaved is true) {
                Console.WriteLine("[ChkDiaryModel] Saving Image Successed");
            }
            else if (ChkSaved is false) {
                Console.WriteLine("[ChkDiaryModel] Saving Image Failed");
            }
            return ChkSaved;
        }


        public bool DeleteDiary() {

            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "GET_DIARY_DETAIL" },
                { "DIARY_UID", _session.GetCurrentDiaryUid() }
            };
            byte[] byteData;

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
            byteData = response.payload;

            /* Parsing json */
            string protocol = jsonData["PROTOCOL"]?.ToString() ?? "";
            string result = jsonData["RESP"]?.ToString() ?? "";

            if( protocol == "DIARY_DEL" &&  result == "SUCCESS" ) {
                return true;
            }
            else if( protocol == "DIARY_DEL" && result == "FAIL" ) {
                return false;
            }
            else {
                return false;
            }
        }

        public bool UpdateDiaryLike(bool isLiked) {
            /*
            {
                PROTOCOL : UPDATE_DIARY_LIKE ,
                DIARY_UID
                IS_LIKED :
            }
             */
            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "UPDATE_DIARY_LIKE" },
                { "DIARY_UID", _session.GetCurrentDiaryUid() },
                { "IS_LIKED", isLiked }
            };
            byte[] byteData;

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
            byteData = response.payload;

            /* Parsing json */
            string protocol = jsonData["PROTOCOL"]?.ToString() ?? "";
            string result = jsonData["RESP"]?.ToString() ?? "";

            if (protocol == "UPDATE_DIARY_LIKE " && result == "SUCCESS") {
                return true;
            }

            else if (protocol == "UPDATE_DIARY_LIKE" && result == "FAIL") {
                return false;
            }

            else {
                return false;
            }
        }
    }
}
