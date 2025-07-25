using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;


namespace MalangDiary.Helpers {
    public class HeartImageConverter : IValueConverter {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
            if (value is bool isLiked) {
                return isLiked
                    ? "/Resources/Images/Icons/heart.png"              // 좋아요 ON
                    : "/Resources/Images/Icons/heart_outline.png";     // 좋아요 OFF
            }

            return "/Resources/Images/Icons/heart_outline.png"; // fallback
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
            throw new NotImplementedException();
        }
    }
}
