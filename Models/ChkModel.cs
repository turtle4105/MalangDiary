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

        /** Constructor **/
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
        private CalendarInfo CalendarInfo;



        /** Member Methods **/

        /* Protocol - GET/SEND_CALENDAR */
        public List<CalendarInfo> GetCalendar(DateTime selectedDateTime) {

            List<CalendarInfo> resultCalendarInfo = new();

            /* [1] new json */
            JObject jsonData = new() {
                { "PROTOCOL", "GET_CALENDAR" },
                { "CHILD_UID", _session.GetCurrentChildUid() },
                { "YEAR", selectedDateTime.Year },
                { "MONTH", selectedDateTime.Month }
            };
            byte[] byteData;

            /* [2] Put json in WorkItem */
            var sendItem = new WorkItem {
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

            if (protocol == "SEND_CALENDAR_REQ" && result == "SUCCESS") {
                Console.WriteLine("[ChkModel] Valid Protocol and Response is Success");

                int cnt = 0;
                JArray jArrayData = new();
                if (jsonData["DATA"] is not null) {
                    jArrayData = jsonData["DATA"]!.ToObject<JArray>();
                }

                foreach (var InfoOfDay in jArrayData) {
                    CalendarInfo tmp_cal_info = new()
                    {
                        Uid = InfoOfDay["DIARY_UID"]!.ToObject<int>(),
                        Date = int.Parse(InfoOfDay["DATE"]!.ToObject<string>()),
                        IsWrited = InfoOfDay["IS_WRITED"]!.ToObject<bool>(),
                        IsLiked = InfoOfDay["IS_LIKED"]!.ToObject<bool>()
                    };

                    resultCalendarInfo.Add(tmp_cal_info);

                    Console.WriteLine($"cnt: {cnt}");
                    Console.WriteLine($"  Uid     = {tmp_cal_info.Uid}");
                    Console.WriteLine($"  Date    = {tmp_cal_info.Date}");
                    Console.WriteLine($"  Writed  = {tmp_cal_info.IsWrited}");
                    Console.WriteLine($"  Liked   = {tmp_cal_info.IsLiked}");

                    cnt++;
                }

                return resultCalendarInfo;
            }
            else if ( protocol == "SEND_CALENDAR_REQ" && result == "FAIL" ) {
                Console.WriteLine("[ChkModel] Valid Protocol but Response is Fail");
                return resultCalendarInfo;
            }
            else if ( protocol != "SEND_CALENDAR_REQ" ) {
                Console.WriteLine("[ChkModel] Invalid Protocol");
                return resultCalendarInfo;
            }
            else {
                return resultCalendarInfo;
            }
        }


        /* Protocol - REQ_GALLERY */
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
                            ResultDiaryInfo.Emotions!.Add(emotion["EMOTION"]!.ToString());
                        }
                    }
                }


                /* Save Audio File  */
                if ( byteData.Length > 0 ) {
                    string FixedImgPath = "./Audio/DiaryAudio.wav";        // set file path
                    _base.WriteWavToFile(FixedImgPath, byteData);          // save wav file
                }
                else if( byteData.Length == 0 ) {
                    Console.WriteLine("[ChkModel] No byteData(Audio)");
                }


                /* Check Image File */
                bool ChkImg = false;
                if( ResultDiaryInfo.PhotoFileName.Length > 0 ) {
                    ChkImg = true;
                    return (ResultDiaryInfo, ChkImg);
                }
                else if( ResultDiaryInfo.PhotoFileName.Length == 0 ) {
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

            JObject jsonData = JObject.Parse(RecvItem.json);
            byte[] byteData = RecvItem.payload;

            string FixedFilePath = "./Images/ChkDiaryImage.jpg";
            
            bool ChkSaved = false;

            if ( byteData.Length > 0 ) {
                ChkSaved = _base.WriteJpgToFile(FixedFilePath, byteData);

                if (ChkSaved is true) {
                    Console.WriteLine("[ChkDiaryModel] Saving Image Successed");
                    ChkSaved = true;
                }
                else if (ChkSaved is false) {
                    Console.WriteLine("[ChkDiaryModel] Saving Image Failed");
                }
            }
            else if ( byteData.Length == 0 ) {
                Console.WriteLine("[ChkModel] No ByteData(JPG)");
            }

            return ChkSaved;
        }


        /* Protocol - DIARY_DEL */
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


        /* Protocol - UPDATE_DIARY_LIKE */
        public bool UpdateDiaryLike(bool isLiked) {

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

            if (protocol == "UPDATE_DIARY_LIKE" && result == "SUCCESS") {
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
