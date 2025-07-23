using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using System;

namespace MalangDiary.ViewModels
{
    public partial class RcdDiaryViewModel : ObservableObject
    {
        public RcdDiaryViewModel(DiaryModel diarymodel)
        {
            _diarymodel = diarymodel;
        }

        private readonly DiaryModel _diarymodel;

        /* 뒤로가기 */
        [RelayCommand]
        private void GoBack()
        {
            Console.WriteLine("[RcdDiaryViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        /* 다시 녹음 */
        [RelayCommand]
        private void ReRecord()
        {
            // 다시 녹음 로직 추가 (예: 페이지 이동 등)
            Console.WriteLine("[RcdDiaryViewModel] 다시 녹음 선택됨");
        }

        /*  새 페이지로 이동: 녹음 종료 버튼 누름 */
        [RelayCommand]
        private void GoToConfirmDiary()
        {
            Console.WriteLine("[RcdDiaryViewModel] 일기 확인 페이지로 이동합니다.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ConfirmDiary));
        }
    }
}
