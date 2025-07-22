using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MalangDiary.ViewModels {
    public partial class HomeViewModel : ObservableObject {
        /* Constructor */
        public HomeViewModel(HomeModel homemodel, UserSession user_session) {
            _homemodel = homemodel;
            _userSession = user_session;
            Console.WriteLine("[HomeModel] UpdateLatestDiary() executed");
            UpdateLatestDiary();
        }

        /** Member Variables **/
        private readonly HomeModel _homemodel;
        private readonly UserSession _userSession;

        /* For Children StackPanel */
        [ObservableProperty] private ObservableCollection<ChildInfo> children;  //자녀 목록 (동적 생성용)
        [ObservableProperty] private int selectedChildUid;          // 	현재 선택된 자녀의 UID
        [ObservableProperty] private int selectedChildIndex;

        /* For Today's Diary */
        [ObservableProperty] private string dateText = DateTime.Now.ToString("yyyy-MM-dd");
        [ObservableProperty] private string weatherText = "none";
        [ObservableProperty] private ObservableCollection<ChildInfo> childrenInfo = new();
        [ObservableProperty] private DiaryInfo latestDiary = new();
        [ObservableProperty] private bool hasDiary;
        [ObservableProperty] private ImageSource? diaryImageSource;

        public ObservableCollection<string> EmotionTags => new(LatestDiary.Emotions?.ToList() ?? []);


        /** Member Methods **/

        /* Methods Related to UI */
        public void UpdateLatestDiary() {
            DiaryInfo result = _homemodel.GetLatestDiary();

            LatestDiary = result;
            HasDiary = result.Uid != 0;

            DateText = LatestDiary.Date ?? DateTime.Now.ToString("yyyy-MM-dd");
            WeatherText = LatestDiary.Weather ?? "날씨 정보 없음";

            DiaryImageSource = LoadImageFromPath(result.PhotoFileName);

            OnPropertyChanged(nameof(EmotionTags));

            return;
        }

        private static string EmotionTagger(string emotion) {
            string tagged_emotion;
            tagged_emotion = "# " + emotion;

            return tagged_emotion;
        }

        private static ImageSource? LoadImageFromPath(string path) {
            Console.WriteLine("LoadImageFromPath Executed");
            Console.WriteLine("path:" + path);
            Console.WriteLine("File.Exists(path):" + File.Exists(path));
            if (!File.Exists(path)) {

                Console.WriteLine("path:" + path);
                Console.WriteLine("File.Exists(path):" + File.Exists(path));
                return null;
            }

            BitmapImage image = new();
            image.BeginInit();
            image.CacheOption = BitmapCacheOption.OnLoad;
            image.UriSource = new Uri(Path.GetFullPath(path), UriKind.Absolute);
            image.EndInit();
            image.Freeze(); // for thread-safety
            return image;
        }

        /* Methods Change Page */
        [RelayCommand]
        public static void GoToRcdDiary() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RcdDiary));
        }

        [RelayCommand]
        public static void GoToChkCld() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkCld));
        }

        [RelayCommand]
        public static void GoToRgsChd() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChd));
        }
    }
}
