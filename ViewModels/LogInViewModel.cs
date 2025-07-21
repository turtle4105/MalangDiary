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
using System.Windows;
using MalangDiary.Models;

namespace MalangDiary.ViewModels {
    // ObservableObject를 상속하면 INotifyPropertyChanged 자동 포함
    public partial class LogInViewModel : ObservableObject {

        public LogInViewModel(UserModel usermodel) {
            _usermodel = usermodel;
        }

        UserModel _usermodel;

        // [ObservableProperty]를 붙이면 자동으로 프로퍼티 + OnPropertyChanged 생성됨
        [ObservableProperty] private string ? email;
        [ObservableProperty] private string ? password;

        // 로그인 버튼 클릭 시 실행되는 명령 - 자동으로 ICommand 구현됨
        [RelayCommand] private void LogIn() {
            // 로그인 시 이메일과 비밀번호가 비어있는지 확인
            if (string.IsNullOrWhiteSpace(Email) || string.IsNullOrWhiteSpace(Password)) {
                MessageBox.Show("이메일과 비밀번호를 입력해주세요.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Model의 LogIn Method 사용
            bool LogInResult = _usermodel.LogIn(Email, Password);
            if( LogInResult is true ) {
                MessageBox.Show($"로그인 성공! {Email}님, 환영합니다.", "로그인 성공", MessageBoxButton.OK, MessageBoxImage.Information);
                GoToHome();
            }
        }

        [RelayCommand]
        private static void GoBack() {
            WeakReferenceMessenger.Default.Send((new PageChangeMessage(PageType.Start)));
        }

        [RelayCommand]
        private static void GoToSignUp() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.SignUp));
        }

        private static void GoToHome() {
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
        }
    }
}
