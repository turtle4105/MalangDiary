using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Models;
using MalangDiary.Services;
using Microsoft.Xaml.Behaviors.Core;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdViewModel : ObservableObject {

        /* Constructor */
        RgsChdViewModel(RgsModel rgsmodel) {
            _rgsmodel = rgsmodel;
        }


        /* Member Variables */
        private readonly RgsModel _rgsmodel;


        /* Member Methods */
        [RelayCommand] private static void GoBack() {
            Console.WriteLine("[RgsChdViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new NavigateBackAction());
        }
    }   
}
