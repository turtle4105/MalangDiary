using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace MalangDiary.Helpers {
    public class PlayPauseTextConverter : IValueConverter {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            bool isPlaying = (bool)value;
            Console.WriteLine($"[Converter] IsPlaying = {isPlaying}");
            return isPlaying ? "❚❚" : "▶";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
            => throw new NotImplementedException();
    }
}
