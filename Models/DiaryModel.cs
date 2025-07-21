using MalangDiary.Services;
using System;
using System.Collections.Generic;
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
    }
}
