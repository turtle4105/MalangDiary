using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Helpers;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace MalangDiary.ViewModels {
    public partial class MdfDiaryViewModel : ObservableObject
    {
        public ObservableCollection<string> WeatherList { get; } = new();

        [ObservableProperty] private string title = string.Empty;
        [ObservableProperty] private string refinedText = string.Empty;
        [ObservableProperty] private ObservableCollection<string> emotionList = new();
        [ObservableProperty] private string date = string.Empty;
        [ObservableProperty] private string photoFileName = string.Empty;
        [ObservableProperty] private string selectedWeather = string.Empty;
        [ObservableProperty] private bool isImageAttached = true;
        [ObservableProperty] private ImageSource? photoPreview; 

        private readonly DiaryModel _diaryModel;
        private string? _selectedImagePath = null;


        public MdfDiaryViewModel(DiaryModel diaryModel)
        {
            _diaryModel = diaryModel;

            for (int i = 1; i <= 8; i++)
            {
                WeatherList.Add(WeatherConveter.ConvertWeatherCodeToText(i));
            }

            SelectedWeather = WeatherList.FirstOrDefault() ?? string.Empty;

            Console.WriteLine("[MdfDiaryViewModel] 생성자 실행됨");
            LoadDiaryInfo();
        }

        public void LoadDiaryInfo()
        {
            if (_diaryModel.CurrentDiaryInfo == null)
            {
                Console.WriteLine("[MdfDiaryViewModel] DiaryInfo 없음");
                return;
            }

            var info = _diaryModel.CurrentDiaryInfo.Value;

            Title = info.Title;
            RefinedText = info.Text;
            Date = info.Date;
            PhotoFileName = info.PhotoFileName;

            EmotionList.Clear();
            foreach (var e in info.Emotions)
                EmotionList.Add(e);

            SelectedWeather = info.Weather;
        }

        partial void OnSelectedWeatherChanged(string value)
        {
            int code = Enumerable.Range(1, 8)
                                 .FirstOrDefault(i => WeatherConveter.ConvertWeatherCodeToText(i) == value);

            if (_diaryModel.CurrentDiaryInfo is not null)
            {
                var diary = _diaryModel.CurrentDiaryInfo.Value;
                diary.IntWeather = code;
                diary.Weather = value;
                _diaryModel.CurrentDiaryInfo = diary;
            }
        }

        [RelayCommand] private void RecAgain() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RcdDiary));
        }

        [RelayCommand]
        private void SaveDiary()
        {
            if (_diaryModel.CurrentDiaryInfo is null)
            {
                Console.WriteLine("[SaveDiary] DiaryInfo 없음");
                return;
            }

            var jsonObj = new
            {
                PROTOCOL = "MODIFY_DIARY",
                TITLE = Title,
                TEXT = RefinedText,
                WEATHER = SelectedWeather,
                IS_LIKED = false,
                PHOTO_FILENAME = PhotoFileName,
                CREATE_AT = Date,
                EMOTIONS = EmotionList.Select(e => new { EMOTION = e }).ToArray()
            };

            string jsonStr = Newtonsoft.Json.JsonConvert.SerializeObject(jsonObj);
            byte[] imageBytes = Array.Empty<byte>();

            if (!string.IsNullOrEmpty(_selectedImagePath) && File.Exists(_selectedImagePath))
            {
                imageBytes = File.ReadAllBytes(_selectedImagePath);
            }

            var item = new WorkItem
            {
                json = jsonStr,
                payload = imageBytes,
                path = _selectedImagePath ?? ""
            };

            _diaryModel.SendModifyDiary(item);
        }
        
        [RelayCommand]
        private void UploadImage()
        {
            Console.WriteLine("사진 첨부 선택");
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "Image Files (*.jpg;*.png)|*.jpg;*.png",
                Title = "이미지 선택"
            };

            if (dialog.ShowDialog() == true)
            {
                string originPath = dialog.FileName;
                string fileName = Path.GetFileName(originPath);
                PhotoFileName = fileName;

                Console.WriteLine($"[UploadImage] 선택된 이미지: {fileName}");

                        // 이미지 임시 저장
                string saveDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "Images");
                Directory.CreateDirectory(saveDir);
                string savePath = Path.Combine(saveDir, fileName);

                byte[] bytes = File.ReadAllBytes(originPath);

                BaseModel baseModel = new BaseModel();
                bool result = baseModel.WriteJpgToFile(savePath, bytes);

                if (result)
                {
                    Console.WriteLine("[UploadImage] 이미지 임시 저장 완료");
                    _selectedImagePath = savePath;

                    var bitmap = new BitmapImage();
                    bitmap.BeginInit();
                    bitmap.CacheOption = BitmapCacheOption.OnLoad;
                    bitmap.UriSource = new Uri(savePath, UriKind.Absolute);
                    bitmap.EndInit();
                    PhotoPreview = bitmap;

                    IsImageAttached = false; 
                }
            }
        }
        [RelayCommand]
        private void GoToHome()
        {
            Console.WriteLine("GoToHome Command Executed");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
        }
    }

}
