using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace MalangDiary.Helpers {
    // 텍스트가 비어 있으면 → 보이게(Visible)
    // 텍스트가 있으면 → 안 보이게(Collapsed)
    // 주로 TextBox에 힌트(Placeholder) 표시할 때 사용
    public class EmptyStringToVisibilityConverter : IValueConverter {
        // 값을 변환하는 함수
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            string text = value as string;
            return string.IsNullOrEmpty(text) ? Visibility.Visible : Visibility.Collapsed;
        }
        // 되돌리는 건 안 쓰므로 예외 던짐
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }
}
