using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MalangDiary.Models {
    public class BaseModel {

        /** Member Methods **/

        /* Check If Image Directory Exists */
        public bool CheckImgDirExists(DirectoryInfo dir) {
            if (dir.Exists is false) {     // No Dir -> Create Dir -> return true
                dir.Create();
                return true;
            }
            else if (dir.Exists is true) { // Yes Dir -> return true
                return true;
            }
            else {
                Console.WriteLine("Failed to Check Dir");
                return false;               // Exception -> return false
            }
        }


        /* Write to File Path */
        public bool WriteJpgToFile(string filePath, byte[] byteArray) {
            string dirPath = "./Images";
            DirectoryInfo dir = new DirectoryInfo(dirPath);
            bool dirCheck = CheckImgDirExists(dir);

            if (!dirCheck) {
                Console.WriteLine("Failed to create Dir");
                return false;
            }

            try {
                File.WriteAllBytes(filePath, byteArray); // 파일이 없으면 자동 생성
                Console.WriteLine($"Jpg has been saved into {filePath}");
                return true;
            }
            catch (IOException ex) {
                Console.WriteLine("파일 저장 중 오류: " + ex.Message);
                return false;
            }
        }




        /* Write to File Path */
        public bool WriteWavToFile(string filePath, byte[] byteArray) {
            string dirPath = "./Audio";
            DirectoryInfo dir = new DirectoryInfo(dirPath);
            bool dirCheck = CheckImgDirExists(dir);

            if (!dirCheck) {
                Console.WriteLine("Failed to create Dir");
                return false;
            }

            try {
                File.WriteAllBytes(filePath, byteArray); // 파일이 없으면 자동 생성
                Console.WriteLine($"Wav has been saved into {filePath}");
                return true;
            }
            catch (IOException ex) {
                Console.WriteLine("파일 저장 중 오류: " + ex.Message);
                return false;
            }
        }
    }
}
