using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Helpers;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using Microsoft.Xaml.Behaviors.Core;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Controls.Primitives;
using System.Windows.Input;

namespace MalangDiary.ViewModels {
    public partial class ChkDiaryViewModel : ObservableObject, INotifyPropertyChanged {

        /* Constructor */
        /** Constructor: 서버에서 데이터 불러오기 (테스트) -> 사진 안 됨 **/
        public ChkDiaryViewModel( ChkModel ? chkmodel, UserSession ? usersession) {
            Console.WriteLine("ChkDiaryViewModel 생성됨");
            // 서버에서 받은 데이터 예시
            ImageList.Add("/Resources/logo/malang_logo.png");

            _chkModel = chkmodel!;  // not null
            _session = usersession!;  // not null

            /* test */
            List<string> test_emotions = ["#기쁨", "#놀람", "#사랑"];
            EmotionList.Add(test_emotions[0]);
            EmotionList.Add(test_emotions[1]);
            EmotionList.Add(test_emotions[2]);
            int test_uid = 1;
            _session.SetCurrentDiaryUid(test_uid);
            /********/

            SetDiary();
        }


        /* Member Variables */
        private readonly ChkModel _chkModel;
        private readonly UserSession _session;

        public event PropertyChangedEventHandler ? PropertyChanged;

        [ObservableProperty] private string dateText;       // date text
        [ObservableProperty] private string weatherText;    // weather text
        [ObservableProperty] private string titleText;      // title text
        [ObservableProperty] private string diaryText;      // diary text
        [ObservableProperty] private bool isPlaying;       // whether playing audio or not

        private DiaryInfo CurrentDiaryInfo;   // diaryinfo

        // 서버에서 받은 이미지 URL 목록
        private ObservableCollection<string> _imageList = [];
        public ObservableCollection<string> ImageList {
            get => _imageList;
            set {
                _imageList = value;
                OnPropertyChanged(nameof(ImageList));
            }
        }

        // 서버에서 받은 감정 태그 목록
        private ObservableCollection<string> _emotionList = [];
        public ObservableCollection<string> EmotionList {
            get => _emotionList;
            set {
                _emotionList = value;
                OnPropertyChanged(nameof(EmotionList));
            }
        }


        /** Member Methods **/

        /* Play Audio */
        [RelayCommand]
        private void PlayPause() {
            if( IsPlaying is true ) {
                //PauseAudio();
            }
            else if( IsPlaying is false ) {
                //PlayAudio();
            }

            IsPlaying = !IsPlaying;
        }

        /* Reset Audio */
        [RelayCommand] private static void RestartAudio() {
            Console.WriteLine("[ChkDiaryViewModel] ResetAudio command executed.");
        }

        /* Share */
        [RelayCommand] private static void ShareDiary() {
            System.Diagnostics.Debug.WriteLine("[ChkDiaryViewModel] Share command executed");
        }

        /* Close */
        [RelayCommand] private static void DeleteDiary() {
            System.Diagnostics.Debug.WriteLine("[ChkDiaryViewModel] Close command executed");
        }

        /* Go Back */
        [RelayCommand]
        private static void GoBack() {
            Console.WriteLine("[ChkDiaryViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(Enums.PageType.Goback));
        }

        /* ONPC */
        [RelayCommand] protected new void OnPropertyChanged(string name) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        /* Set Diary */
        private void SetDiary() {
            int cur_diary_uid = _session.GetCurrentDiaryUid();
            CurrentDiaryInfo = _chkModel.GetDiaryDetail(cur_diary_uid);

            DateText    = CurrentDiaryInfo.Date;
            WeatherText = CurrentDiaryInfo.Weather;
            TitleText   = CurrentDiaryInfo.Title;
            DiaryText   = CurrentDiaryInfo.Text;
        }
    }
}
