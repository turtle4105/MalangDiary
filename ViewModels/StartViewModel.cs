using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.ViewModels;

public partial class StartViewModel : ObservableObject {
    [RelayCommand]
    private static void GoToLogin() {
        WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Login));
        Console.WriteLine("GoToLogin command executed.");
    }

    [RelayCommand]
    private static void GoToSignUp() {
        WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.SignUp));
        Console.WriteLine("GoToSignUp command executed.");
    }
}