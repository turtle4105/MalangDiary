using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MalangDiary.Views {
    /// <summary>
    /// Page1.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class LogInView : Page {
        public LogInView() {
            InitializeComponent();
        }
        private void PasswordBox_PasswordChanged(object sender, RoutedEventArgs e) {
            if (sender is PasswordBox passwordBox) {
                if (FindName("PasswordTextBlock") is TextBlock textBlock) {
                    // 비밀번호가 입력되면 TextBlock을 숨기고, 비어 있으면 보이도록 설정
                    textBlock.Visibility = string.IsNullOrEmpty(passwordBox.Password) ? Visibility.Visible : Visibility.Collapsed;
                }
            }
        }
    }
}
