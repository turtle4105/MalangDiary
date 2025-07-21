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
    public partial class HomeViewModel {

        public HomeViewModel(HomeModel homemodel) {
            //_homemodel = homemodel;
        }

        //HomeModel _homemodel = 

        [RelayCommand] public static void GoToRcdDiary() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RcdDiary));
        }

        [RelayCommand] public static void GoToChkCld() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkCld));
        }

        [RelayCommand]
        public static void GoToRgsChd() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChd));
        }
    }
}
