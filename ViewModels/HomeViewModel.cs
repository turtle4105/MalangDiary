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
        public HomeViewModel(UserModel usermodel,HomeModel homemodel, UserSession user_session) {
            _usermodel = usermodel;
            _homemodel = homemodel;
            _session = user_session;
            Console.WriteLine("[HomeModel] UpdateLatestDiary() executed");
            LoadChildrenFromModel();
            UpdateLatestDiary();
        }

        /** Member Variables **/
        private readonly UserModel _usermodel;
        private readonly HomeModel _homemodel;
        private readonly UserSession _session;

        /* For Top */
        [ObservableProperty] private string? selectedChildIconColor;

        /* For Children StackPanel */
        public ObservableCollection<ChildViewModel> Children { get; } = new();  //자녀 목록 (동적 생성용)
        [ObservableProperty] private int selectedChildUid;          // 	현재 선택된 자녀의 UID


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


        // 나머지 객체의 IsSelected를 false로, 선택한 객체의 IsSelected만 true로
        [RelayCommand]
        private void SelectChild(ChildViewModel selected) {
            foreach (var child in Children)
                child.IsSelected = false;

            selected.IsSelected = true;
            SelectedChildUid = selected.Uid;

            // 세션 정보 갱신
            _session.SetCurrentChildUid(selected.Uid);

            // 유저 모델 갱신
            ChildInfo selected_childinfo = new()
            {
                Uid = selected.Uid,
                Name = selected.Name,
                Age = selected.Age,
                BirthDate = selected.BirthDate,
                Gender = selected.Gender,
                IconColor = selected.IconColor
            };
            _usermodel.SetSelectedChildInfo(selected_childinfo);

            // 선택된 자녀색상 Binding 객체 값 변경
            SelectedChildIconColor = _usermodel.GetSelectedChildInfo().IconColor;
            Console.WriteLine("_usermodel.GetSelectedChildInfo().IconColor:" + _usermodel.GetSelectedChildInfo().IconColor);

            // 오늘 일기 불러오기
            UpdateLatestDiary();
        }


        public void LoadChildrenFromModel() {
            Children.Clear();
            foreach (var child in _usermodel.GetAllChildInfo()) {
                var vm = new ChildViewModel(child);
                if (child.Uid == _session.GetCurrentChildUid())
                    vm.IsSelected = true;
                Children.Add(vm);
            }

            SelectedChildIconColor = _usermodel.GetAllChildInfo()[0].IconColor;
        }


        /* Methods Change Page */
        [RelayCommand] public static void GoToRcdDiary() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RcdDiary));
        }


        [RelayCommand] public static void GoToChkCld() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkCld));
        }


        [RelayCommand] public static void GoToRgsChd() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChd));
        }
    }


    public partial class ChildViewModel : ObservableObject {
        // [OP] isSelected를 사용하기 위해 ChildViewModel Class 선언
        // Struct ChildInfo를 생성자로 넣고, 현재 선택된 자녀 Uid와 param구조체의 Uid를 비교해,
        // IsSelected bool값 조정
        public int Uid { get; }
        public string Name { get; }
        public string BirthDate { get; }
        public int Age { get; }
        public string Gender { get; }
        public string IconColor { get; }

        [ObservableProperty]
        private bool isSelected;

        public ChildViewModel(ChildInfo child) {
            // Struct ChildInfo를 Param으로 받아, 각 값을 할당
            Uid = child.Uid;
            Name = child.Name;
            BirthDate = child.BirthDate;
            Age = child.Age;
            Gender = child.Gender;
            IconColor = child.IconColor;
            IsSelected = false;     // Default = false
        }
    }
}