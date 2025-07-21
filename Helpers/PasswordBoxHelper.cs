using System.Windows;
using System.Windows.Controls;

namespace MalangDiary.Helpers {
    public static class PasswordBoxHelper {
        public static readonly DependencyProperty BoundPasswordProperty =
            DependencyProperty.RegisterAttached("BoundPassword", typeof(string), typeof(PasswordBoxHelper),
                new PropertyMetadata(string.Empty, OnBoundPasswordChanged));

        public static string GetBoundPassword(DependencyObject obj) =>
            (string)obj.GetValue(BoundPasswordProperty);

        public static void SetBoundPassword(DependencyObject obj, string value) =>
            obj.SetValue(BoundPasswordProperty, value);

        private static void OnBoundPasswordChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) {
            if (d is PasswordBox passwordBox) {
                passwordBox.PasswordChanged -= PasswordBox_PasswordChanged;

                if (!GetIsUpdating(passwordBox))
                    passwordBox.Password = (string)e.NewValue;

                passwordBox.PasswordChanged += PasswordBox_PasswordChanged;
            }
        }

        private static readonly DependencyProperty IsUpdatingProperty =
            DependencyProperty.RegisterAttached("IsUpdating", typeof(bool), typeof(PasswordBoxHelper));

        private static bool GetIsUpdating(DependencyObject obj) =>
            (bool)obj.GetValue(IsUpdatingProperty);

        private static void SetIsUpdating(DependencyObject obj, bool value) =>
            obj.SetValue(IsUpdatingProperty, value);

        // PasswordBox의 비밀번호가 변경될 때 TextBlock의 Visibility를 업데이트
        private static void PasswordBox_PasswordChanged(object sender, RoutedEventArgs e) {
            var passwordBox = sender as PasswordBox;

            SetIsUpdating(passwordBox, true);

            SetBoundPassword(passwordBox, passwordBox.Password);

            SetIsUpdating(passwordBox, false);

            // PasswordBox의 비밀번호가 변경되었을 때 TextBlock의 Visibility 업데이트
            var parentGrid = passwordBox.Parent as Grid;
            var textBlock = parentGrid?.Children.OfType<TextBlock>().FirstOrDefault(tb => tb.Name == "PasswordTextBlock");
            if (textBlock != null) {
                textBlock.Visibility = string.IsNullOrEmpty(passwordBox.Password) ? Visibility.Visible : Visibility.Collapsed;
            }
        }
    }
}
