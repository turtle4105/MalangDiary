using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace MalangDiary.Helpers {
    // 정수 값이 0이면 → 보이게(Visible)
    // 1 이상이면 → 안 보이게(Collapsed)
    // 주로 TextBox.Text.Length에 바인딩해서 힌트 표시용으로 사용
    public class IntToVisibilityConverter : IValueConverter {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            // value가 정수면 그대로 쓰고, 아니면 0으로 처리
            int length = value is int i ? i : 0;
            return length == 0 ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }
}

