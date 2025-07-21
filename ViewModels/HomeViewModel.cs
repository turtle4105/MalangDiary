using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls.Primitives;

namespace MalangDiary.ViewModels
{
    public partial class HomeViewModel : ObservableObject {

        /* Constructor (생성자) */
        public HomeViewModel(HomeModel homemodel) {
            _homemodel = homemodel;
            GetLatestDiary();
        }

        HomeModel _homemodel;

        [ObservableProperty] private string dateText = "2024-07-21";
        [ObservableProperty] private string weatherText = "12:34:56";

        public void GetLatestDiary() {
            // 여기에 최신 일기를 가져오는 로직을 추가합니다.
            // 예시로, 현재 날짜와 시간을 업데이트합니다.
            DiaryInfo LatestDiaryInfo = _homemodel.GetDiaryInfo();
            DateText = DateTime.Now.ToString("yyyy-MM-dd");
            if ( LatestDiaryInfo.Uid == 0) {                 // 일기가 없는 경우
                WeatherText = "날씨 정보 없음";
            } else {
                // 일기가 있는 경우
                DateText = LatestDiaryInfo.Date;
                WeatherText = "일기가 없습니다.";
            }
        }

        [RelayCommand] public static void GoToRcdDiary() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RcdDiary));
        }

        [RelayCommand] public static void GoToChkCld() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkCld));
        }

        [RelayCommand]
        public static void GoToRgsChd() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChd));
        }
    }
}
