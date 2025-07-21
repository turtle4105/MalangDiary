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
        private DiaryInfo LatestDiary; // 최신 일기 정보


        /* Member Mehtods */
        public void LoadData() {
            GetLatestDiary();
            GetBabyInfo();
        }


        public void GetLatestDiary() {
            if (_session.GetCurrentChildUid() == 0) {
                Console.WriteLine("[HomeModel] 현재 선택된 아기가 없습니다.");
                return;
            }
            // 현재 선택된 아기의 UID를 가져옵니다.
            int TmpCurrentChildUid = _session.GetCurrentChildUid();
            string CurrentChildUid = TmpCurrentChildUid.ToString();

            // json 생성
            JObject jsonData = new()
            {
                {"PROTOCOL", "GET_LATEST_DIARY" },
                {"CHILDUID", CurrentChildUid }
            };

            // 보내는 메세지작업
            WorkItem sendingItem = new WorkItem
            {
                json = jsonData.ToString(),
                payload = { },
                path = { }
            };

            _socket.Send(sendingItem);

            // 받는 작업( 임시 )
            jsonData = JObject.Parse(_socket.Receive().json);

            string protocol = jsonData["PROTOCOL"]!.ToString();
            string response = jsonData["RESP"]!.ToString();   // null 아님

            if (protocol == "GET_LATEST_DIARY" && response == "SUCCESS") {
                
                LatestDiary.Title = jsonData["TITLE"]!.ToString();
                LatestDiary.Weather = jsonData["WEATHER"]!.ToObject<int>();
                LatestDiary.Date = jsonData["CREATE_AT"]!.ToString();
                if (jsonData["EMOTIONS"] is not null ) {
                    for (int i = 0; i < 5; i++) {
                        LatestDiary.Emotions[i] = jsonData["EMOTIONS"]!["EMOTION"]!.ToString();
                    }
                }
            }

            return;
        }

        public static void GetBabyInfo() {

        }
    }
}
