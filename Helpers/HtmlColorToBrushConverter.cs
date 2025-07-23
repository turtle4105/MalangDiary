using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Media;

namespace MalangDiary.Helpers {
    public class HtmlColorToBrushConverter : IValueConverter {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            if (value is string htmlColor && !string.IsNullOrWhiteSpace(htmlColor)) {
                return (SolidColorBrush) new BrushConverter().ConvertFromString(htmlColor);
            }
            // Default = LightGray
            return new SolidColorBrush(Colors.LightGray);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
            throw new NotImplementedException();
        }
    }
}
