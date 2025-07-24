using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.ViewModels
{
    public partial class ConfirmDiaryViewModel : ObservableObject
    {

        public ConfirmDiaryViewModel(DiaryModel diarymodel)
        {
            _diarymodel = diarymodel;
        }
        DiaryModel _diarymodel;

        // 다시 녹음 
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

        [RelayCommand]
        private async Task CreateAndSendDiaryAsync()
        {
            CreateDiary();  // 현재 메시지 전송
            await _diarymodel.SendDiaryAsync();  // 실제 송신
        }

    }
}
