using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Helpers;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using Microsoft.Xaml.Behaviors.Core;
using NAudio.CoreAudioApi;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
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
            //int test_uid = 1;
            //_session.SetCurrentDiaryUid(test_uid);
            /********/

            SetDiary();
        }


        /** Member Variables **/
        private readonly ChkModel _chkModel;
        private readonly UserSession _session;

        public new event PropertyChangedEventHandler ? PropertyChanged;

        [ObservableProperty] private string dateText;       // date text
        [ObservableProperty] private string weatherText;    // weather text
        [ObservableProperty] private string titleText;      // title text
        [ObservableProperty] private string diaryText;      // diary text
        [ObservableProperty] private bool isPlaying;        // whether playing audio or not
        [ObservableProperty] private bool isLiked;

        private DiaryInfo CurrentDiaryInfo;   // diaryinfo

        /* Img List from Server */
        private ObservableCollection<string> _imageList = [];
        public ObservableCollection<string> ImageList {
            get => _imageList;
            set {
                _imageList = value;
                OnPropertyChanged(nameof(ImageList));
            }
        }

        /* Emotion Tags from Server */
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
        [RelayCommand] private void PlayPause() {
            if( IsPlaying is true ) {
                //PauseAudio();
            }
            else if( IsPlaying is false ) {
                //PlayAudio();
            }

            IsPlaying = !IsPlaying;
        }

        /* Reset Audio */
        [RelayCommand] private static void StopAudio() {
            Console.WriteLine("[ChkDiaryViewModel] ResetAudio command executed.");
        }

        /* Share Diary */
        [RelayCommand] private static void ShareDiary() {
            System.Diagnostics.Debug.WriteLine("[ChkDiaryViewModel] Share command executed");
        }

        /* Delete Diary */
        [RelayCommand] private void DeleteDiary() {
            System.Diagnostics.Debug.WriteLine("[ChkDiaryViewModel] Close command executed");
            bool ChkDel = _chkModel.DeleteDiary();
            if( ChkDel is true ) {
                Console.WriteLine("[ChkDiaryViewModel] DeleteDiary Successed");
                WeakReferenceMessenger.Default.Send<PageChangeMessage>(new PageChangeMessage(Enums.PageType.Goback));
            }
            else if ( ChkDel is true ) {
                Console.WriteLine("[ChkDIaryViewModel] DeleteDiary Failed");
            }
        }

        /* Change IsLiked */
        [RelayCommand] private void ChangeIsLiked() {
            bool ChkUpdate = false;
            if( IsLiked is true ) {
                IsLiked = false;
                ChkUpdate = _chkModel.UpdateDiaryLike(IsLiked);
            }
            else if ( IsLiked is false ) {
                IsLiked = true;
                ChkUpdate = _chkModel.UpdateDiaryLike(IsLiked);
            }
            if (ChkUpdate is true) {
                Console.WriteLine("[ChkDiaryViewModel] UpdateDiaryLike Successed");
            }
            else if (ChkUpdate is false) {
                Console.WriteLine("[ChkDIaryViewModel] UpdateDiaryLike Failed");
            }

            return;
        }

        /* Go Back */
        [RelayCommand] private static void GoBack() {
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
            bool ImgExist = false;
            (CurrentDiaryInfo, ImgExist) = _chkModel.GetDiaryDetail(cur_diary_uid);

            /* ObservableProperties Assignment */
            DateText = CurrentDiaryInfo.Date;
            WeatherText = CurrentDiaryInfo.Weather;
            TitleText   = CurrentDiaryInfo.Title;
            DiaryText   = CurrentDiaryInfo.Text;
            IsLiked     = CurrentDiaryInfo.IsLiked;
            foreach (var diary in CurrentDiaryInfo.Emotions) {
                EmotionList.Add(diary);
            }
            
            if( ImgExist ) {
                /* Recv Image From Server */
                _chkModel.GetDiaryImage();

                /* Binding Image From Local */
                LoadJpgFromLocal();
            }
        }


        /* Load JPG from local file and set to ImageList */
        private bool LoadJpgFromLocal() {
            string imagePath = System.IO.Path.GetFullPath("./Images/ChkDiaryImage.jpg");

            if (File.Exists(imagePath)) {
                ImageList.Clear();             // 기존 이미지 제거
                ImageList.Add(imagePath);      // 새 이미지 추가
                Console.WriteLine($"[ChkDiaryViewModel] Image loaded: {imagePath}");
                return true;
            }
            else {
                Console.WriteLine("[ChkDiaryViewModel] Image file not found.");
                return false;
            }
        }

    }
}
