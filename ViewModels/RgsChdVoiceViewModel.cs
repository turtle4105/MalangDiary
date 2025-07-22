using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MalangDiary.Models;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Messages;
using MalangDiary.Enums;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdVoiceViewModel : ObservableObject {
    
        /* Constructor */
        public RgsChdVoiceViewModel(RgsModel rgsmodel) {
            _rgsmodel = rgsmodel;
        }
        
        private readonly RgsModel _rgsmodel;

        /* Member Methods */
        [RelayCommand] private static void GoBack() {
            Console.WriteLine("[RgsChdVoiceViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }
    }
}
