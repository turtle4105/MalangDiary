using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Models;
using Microsoft.Xaml.Behaviors.Core;
using System.Collections.ObjectModel;
using System.Windows.Input;

namespace MalangDiary.ViewModels {
    public class PhotoItem {
        public string ImagePath { get; set; }
    }

    public partial class ChkGalleryViewModel : ObservableObject {

        public ObservableCollection<PhotoItem> PhotoList { get; set; }

        public ICommand PhotoClickCommand { get; set; }

        /* Constructor */
        public ChkGalleryViewModel(ChkModel chkmodel) {

            _chkmodel = chkmodel;
            PhotoList = new ObservableCollection<PhotoItem>();

            // 기본 틀 21칸 확보 (7행 x 3열)
            for (int i = 0; i < 21; i++) {
                PhotoList.Add(new PhotoItem());  // 이미지 없는 빈 칸
            }

            PhotoClickCommand = new RelayCommand<PhotoItem>(OnPhotoClick);
        }

        /* Member Variables */
        private readonly ChkModel _chkmodel;


        /* Member Methods */
        private void OnPhotoClick(PhotoItem photo) {
            // 이미지 없는 칸은 클릭 무시
            if (string.IsNullOrEmpty(photo?.ImagePath))
                return;

            // 이미지 있는 칸 클릭 처리
            // TODO: 일기 보기 화면 이동
        }

        /// <summary>
        /// 서버에서 이미지 수신 시 호출
        /// </summary>
        public void AddPhoto(string imagePath) {
            // 첫 번째 빈 칸을 찾아서 대체
            var empty = PhotoList.FirstOrDefault(p => string.IsNullOrEmpty(p.ImagePath));
            if (empty != null) {
                empty.ImagePath = imagePath;
            }
            else {
                // 다 찼으면 새로 추가해서 스크롤 확장
                PhotoList.Add(new PhotoItem { ImagePath = imagePath });
            }
        }

        [RelayCommand] private void GoBack() {
            Console.WriteLine("[ChkGalleryViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new NavigateBackAction());
        }
    }
}
