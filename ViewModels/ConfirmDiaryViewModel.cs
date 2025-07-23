using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using System;

namespace MalangDiary.ViewModels
{
    public partial class ConfirmDiaryViewModel : ObservableObject
    {
        // 다시 녹음 (= 이전으로 되돌리기)
        [RelayCommand]
        private void ReRecord()
        {
            Console.WriteLine("[ConfirmDiaryViewModel] 다시 녹음 선택됨 → 이전 화면으로 이동");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        // 일기 생성
        [RelayCommand]
        private void CreateDiary()
        {
            Console.WriteLine("[ConfirmDiaryViewModel] 일기 생성 선택됨");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.GenDiary));
        }
    }
}
