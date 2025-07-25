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
            Console.WriteLine("[ConfirmDiaryViewModel] 생성자 호출됨");
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
        private void CreateAndSendDiary()
        {
            Console.WriteLine("[ConfirmDiaryViewModel] 일기 생성 요청 + 수신 대기 시작");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.GenDiary));

            _diarymodel.SendDiaryAsync();         // 음성 + JSON 전송
            _diarymodel.StartListening();         // 수신하고 페이지 전환
        }


    }
}
