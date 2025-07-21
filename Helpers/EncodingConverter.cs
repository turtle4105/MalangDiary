using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Helpers {
    public class EncodingConverter {
        public static string CP949(byte[] utf8Bytes) {
            
            // CP949 인코딩으로 변환
            byte[] cp949Bytes = Encoding.Convert(Encoding.UTF8, Encoding.GetEncoding("CP949"), utf8Bytes);

            // CP949 바이트 배열을 문자열로 디코딩
            return Encoding.GetEncoding("CP949").GetString(cp949Bytes);
        }
    }
}
