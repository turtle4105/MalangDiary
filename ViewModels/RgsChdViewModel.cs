using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using System;
using System.IO;
using System.Windows;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdViewModel : ObservableObject
    {
        /* Constructor */
        public RgsChdViewModel(RgsModel rgsModel)
        {
            Console.WriteLine("RgsChdViewModel has Created");
            _rgsModel = rgsModel;
            SelectedProfileColor = "#FFACAC"; // 기본값

            // 녹음 완료 메시지 수신 등록
            WeakReferenceMessenger.Default.Register<VoiceRecordedMessage>(this, (r, m) => {
                if (m.IsRecorded)
                {
                    IsVoiceRecorded = true;
                    Console.WriteLine("[RgsChdViewModel] 녹음 완료 상태 반영됨");
                }
            });
        }

        [ObservableProperty] private string name;
        [ObservableProperty] private string birthYear;
        [ObservableProperty] private string birthMonth;
        [ObservableProperty] private string birthDay;
        [ObservableProperty] private string selectedProfileColor;
        [ObservableProperty] private bool isVoiceRecorded;

        /* Member Variables */
        private readonly RgsModel _rgsModel;

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

        /* Member Methods */

        public void ClearForm()
        {
            Name = string.Empty;
            BirthYear = string.Empty;
            BirthMonth = string.Empty;
            BirthDay = string.Empty;
            IsMale = false;
            IsFemale = false;
            SelectedProfileColor = "#FFACAC";
            IsVoiceRecorded = false;
        }

        [RelayCommand] private static void GoBack() {
            Console.WriteLine("[RgsChdViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }


        /* SetColor */
        [RelayCommand] public void SetColor(string colorCode)
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

            // 입력값 검증
            if (string.IsNullOrWhiteSpace(Name) ||
                string.IsNullOrWhiteSpace(birthdate) ||
                string.IsNullOrWhiteSpace(gender))
            {
                Console.WriteLine(" 입력값 누락: 이름, 생년월일, 성별 중 하나 이상 없음");
                MessageBox.Show("이름, 생년월일, 성별을 모두 입력해주세요.", "입력 누락", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (!IsVoiceRecorded)
            {
                MessageBox.Show("아이의 목소리를 등록해주세요.", "녹음 누락", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            //  자녀 등록
            var (childSuccess, childMsg) = _rgsModel.RegisterChild(Name, birthdate, gender, SelectedProfileColor);
            if (!childSuccess)
            {
                MessageBox.Show("자녀 등록에 실패했습니다.", "등록 실패", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            Console.WriteLine($"자녀 등록 성공: {childMsg}");

            // 목소리 등록 (등록된 CHILD_UID 기반)
            string voiceFilePath = Path.Combine("recordings", "setting_voice.wav");
            var (voiceSuccess, voiceMsg) = _rgsModel.SetBabyVoice(voiceFilePath);

            if (!voiceSuccess)
            {
                MessageBox.Show("목소리 등록에 실패했습니다.", "등록 실패", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            Console.WriteLine($"목소리 등록 성공: {voiceMsg}");

            //  최종 완료
            MessageBox.Show("자녀와 목소리 등록이 모두 완료되었습니다!", "등록 성공", MessageBoxButton.OK, MessageBoxImage.Information);
            ClearForm();
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
        }


        [RelayCommand]
        public void RecordVoice()
        {   
            // 아이 목소리 등록하기 View로 이동
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChdVoice));
        }
    }
}
