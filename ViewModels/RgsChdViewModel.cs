using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MalangDiary.Models;
using System;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdViewModel : ObservableObject
    {
        private readonly RgsModel _rgsModel;

        public RgsChdViewModel(RgsModel rgsModel)
        {
            Console.WriteLine("✅ RgsChdViewModel has Created");
            _rgsModel = rgsModel;
            SelectedProfileColor = "#FFACAC"; // 기본값
        }

        [ObservableProperty] private string name;
        [ObservableProperty] private string birthYear;
        [ObservableProperty] private string birthMonth;
        [ObservableProperty] private string birthDay;
        [ObservableProperty] private string selectedProfileColor;

        private bool isMale;
        public bool IsMale
        {
            get => isMale;
            set
            {
                if (SetProperty(ref isMale, value) && value)
                {
                    IsFemale = false;
                }
            }
        }

        private bool isFemale;
        public bool IsFemale
        {
            get => isFemale;
            set
            {
                if (SetProperty(ref isFemale, value) && value)
                {
                    IsMale = false;
                }
            }
        }

        [RelayCommand]
        public void SetColor(string colorCode)
        {
            SelectedProfileColor = colorCode;
            Console.WriteLine($" 프로필 색상 선택됨: {colorCode}");
        }

        [RelayCommand]
        public void RgsChd()
        {
            Console.WriteLine(" [RgsChd] 자녀 등록 시도됨");

            string gender = IsMale ? "M" : IsFemale ? "F" : "";
            string birthdate = $"{BirthYear}-{BirthMonth}-{BirthDay}";

            if (string.IsNullOrWhiteSpace(Name) || string.IsNullOrWhiteSpace(birthdate) || string.IsNullOrWhiteSpace(gender))
            {
                Console.WriteLine(" 입력값 누락: 이름, 생년월일, 성별 중 하나 이상 없음");
                return;
            }

            var (isSuccess, message) = _rgsModel.RegisterChild(Name, birthdate, gender, SelectedProfileColor);

            if (isSuccess)
                Console.WriteLine($" 자녀 등록 성공: {message}");
            else
                Console.WriteLine($" 자녀 등록 실패: {message}");
        }

        [RelayCommand]
        public void RecordVoice()
        {
            Console.WriteLine(" [RecordVoice] 녹음 기능 호출됨 (아직 구현 안 됨)");
        }
    }
}
