using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace MalangDiary.Helpers {
    public class WritedToBackgroundConverter : IValueConverter {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            if (value is bool isWrited && isWrited)
                return new SolidColorBrush((Color)ColorConverter.ConvertFromString("#FFF9F0"));
            else
                return Brushes.Transparent;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }

}
