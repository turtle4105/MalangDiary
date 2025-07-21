using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Services;
using Microsoft.Extensions.DependencyInjection;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Xml.Linq;

namespace MalangDiary.ViewModels {
    // ObservableObject를 상속하면 INotifyPropertyChanged 자동 포함
    public partial class SignUpViewModel(UserModel userModel, UserSession session) : ObservableObject {

        // [ObservableProperty]를 붙이면 자동으로 프로퍼티 + OnPropertyChanged 생성됨
        [ObservableProperty] private string name;      // Binding 대상이 Name이면, OP는 소문자로 작성
        [ObservableProperty] private string email;
        [ObservableProperty] private string password;
        [ObservableProperty] private string phone;

        // 버튼 클릭 시 실행되는 명령 - 자동으로 ICommand 구현됨
        [RelayCommand]
        private void SignUp() {

            var (isSuccess, message) = userModel.RegisterParents(Name, Email, Password, Phone);

            if (!isSuccess) {
                MessageBox.Show(message, "회원가입 실패", MessageBoxButton.OK, MessageBoxImage.Error);
            }
            else if (isSuccess) {
                MessageBox.Show($"{Name}님, 회원가입이 완료되었습니다!", "회원가입 성공", MessageBoxButton.OK, MessageBoxImage.Information);
                GoToLogIn();
            }

            Name = string.Empty;
            Email = string.Empty;
            Password = string.Empty;
            Phone = string.Empty;

            return;
        }


        [RelayCommand]
        private static void GoBack() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Start));
            Console.WriteLine("GoBack command executed.");
        }

        [RelayCommand]
        private static void GoToLogIn() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Login));
        }
    }
}

