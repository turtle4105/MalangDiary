using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Helpers
{
    public class WeatherConveter
    {
        public static string ConvertWeatherCodeToText(int weatherCode)
        {
            return weatherCode switch
            {
                1 => "맑음",
                2 => "구름 조금",
                3 => "구름 많음",
                4 => "흐림",
                5 => "비",
                6 => "눈",
                7 => "비/눈",
                8 => "안개",
                _ => "알 수 없음"
            };
        }
    }
}

