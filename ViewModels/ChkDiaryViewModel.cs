using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Input;
using MalangDiary.Helpers;

namespace MalangDiary.ViewModels {
    public class ChkDiaryViewModel : INotifyPropertyChanged {

        public event PropertyChangedEventHandler PropertyChanged;

        public ICommand PlayCommand { get; set; }
        public ICommand ShareCommand { get; set; }
        public ICommand CloseCommand { get; set; }

        // 서버에서 받은 이미지 URL 목록
        private ObservableCollection<string> _imageList = new ObservableCollection<string>();
        public ObservableCollection<string> ImageList {
            get => _imageList;
            set {
                _imageList = value;
                OnPropertyChanged(nameof(ImageList));
            }
        }

        // 서버에서 받은 감정 태그 목록
        private ObservableCollection<string> _emotionList = new ObservableCollection<string>();
        public ObservableCollection<string> EmotionList {
            get => _emotionList;
            set {
                _emotionList = value;
                OnPropertyChanged(nameof(EmotionList));
            }
        }

        // 생성자: 서버에서 데이터 불러오기 (테스트) -> 사진 안 됨
        public ChkDiaryViewModel() {
            Console.WriteLine("ChkDiaryViewModel 생성됨");
            // 서버에서 받은 데이터 예시
            ImageList.Add("/Resources/logo/malang_logo.png");

            EmotionList.Add("#기쁨");
            EmotionList.Add("#놀람");
            EmotionList.Add("#사랑");

            // 명령 연결
            PlayCommand = new RelayCommand(o => Play());
            ShareCommand = new RelayCommand(o => Share());
            CloseCommand = new RelayCommand(o => Close());

        }

        private void Play() {
            System.Diagnostics.Debug.WriteLine("재생 커맨드 실행됨");
        }

        private void Share() {
            System.Diagnostics.Debug.WriteLine("공유 커맨드 실행됨");
        }

        private void Close() {
            System.Diagnostics.Debug.WriteLine("닫기 커맨드 실행됨");
        }

        protected void OnPropertyChanged(string name) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
