using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MalangDiary.Models;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Messages;
using MalangDiary.Enums;

namespace MalangDiary.ViewModels {
    public partial class MdfDiaryViewModel : ObservableObject
    {
        public MdfDiaryViewModel( DiaryModel diarymodel ) {
            _diaryModel = diarymodel;
        }

        DiaryModel _diaryModel;

        
        [RelayCommand] private static void RecAgain() {
            // 녹음 다시하기 버튼 클릭 시 실행되는 메서드
        }
        [RelayCommand] private static void SaveDiary() {
            // 일기 저장 버튼 클릭 시 실행되는 메서드
        }
        [RelayCommand] private static void CancelDiary() {
            // 일기 취소 버튼 클릭 시 실행되는 메서드
        }
        [RelayCommand] private static void UploadImage() {
            // 이미지 업로드 버튼 클릭 시 실행되는 메서드
        }

        [RelayCommand] private static void GoToHome () {
            Console.WriteLine("GoToHome Command Executed");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
        }
    }
}
