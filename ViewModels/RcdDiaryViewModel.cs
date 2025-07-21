using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Structs;
using Microsoft.Xaml.Behaviors.Core;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.ViewModels
{
    public partial class RcdDiaryViewModel : ObservableObject {
        public RcdDiaryViewModel(DiaryModel diarymodel) {
            _diarymodel = diarymodel;
        }
        private readonly DiaryModel _diarymodel;

        [RelayCommand] private void GoBack() {
            Console.WriteLine("[RcdDiaryViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }
    }
}
