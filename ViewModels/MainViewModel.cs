using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using MalangDiary.Views;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Forms;
namespace MalangDiary.ViewModels;

public class MainViewModel {
    private Frame? _mainFrame;
    private readonly DiaryModel _diaryModel;

    public MainViewModel(DiaryModel diaryModel) {
        _diaryModel = diaryModel;
        _diaryModel.StartListening();

        WeakReferenceMessenger.Default.Register<PageChangeMessage>(this, (r, m) =>
        {
            Navigate(m.Value);
        });
        
    }

    public void SetFrame(Frame frame) {  // 메인 프레임 설정(예: MainWindow.xaml.cs에서 호출)
        _mainFrame = frame;              // 프레임을 설정
        Navigate(PageType.Start);  // 첫 페이지 로딩 ( Enum PageType.Start )
    }

    private void Navigate(PageType page) {
        if (_mainFrame == null) return;

        switch (page) {
            case PageType.Start:
                _mainFrame.Navigate(new StartView { DataContext = App.Services!.GetService(typeof(StartViewModel)) });
                break;
            case PageType.Login:
                _mainFrame.Navigate(new LogInView { DataContext = App.Services!.GetService(typeof(LogInViewModel)) });
                break;
            case PageType.SignUp:
                _mainFrame.Navigate(new SignUpView { DataContext = App.Services!.GetService(typeof(SignUpViewModel)) });
                break;
            case PageType.Home:
                _mainFrame.Navigate(new HomeView { DataContext = App.Services!.GetService(typeof(HomeViewModel)) });
                break;
            case PageType.RgsChd:
                _mainFrame.Navigate(new RgsChdView { DataContext = App.Services!.GetService(typeof(RgsChdViewModel)) });
                break;
            case PageType.RgsChdVoice:
                _mainFrame.Navigate(new RgsChdVoiceView { DataContext = App.Services!.GetService(typeof(RgsChdVoiceViewModel)) });
                break;
            case PageType.RcdDiary:
                _mainFrame.Navigate(new RcdDiaryView { DataContext = App.Services!.GetService(typeof(RcdDiaryViewModel)) });
                break;
            case PageType.MdfDiary:
                _mainFrame.Navigate(new MdfDiaryView { DataContext = App.Services!.GetService(typeof(MdfDiaryViewModel)) });
                break;
            case PageType.ChkCld:
                _mainFrame.Navigate(new ChkCldView { DataContext = App.Services!.GetService(typeof(ChkCldViewModel)) });
                break;
            case PageType.ChkGallery:
                _mainFrame.Navigate(new ChkGalleryView { DataContext = App.Services!.GetService(typeof(ChkGalleryViewModel)) });
                break;
            case PageType.ChkDiary:
                _mainFrame.Navigate(new ChkDiaryView { DataContext = App.Services!.GetService(typeof(ChkDiaryViewModel)) });
                break;
            case PageType.Goback:
                _mainFrame.GoBack();
                break;
            case PageType.ConfirmDiary:
                _mainFrame.Navigate(new ConfirmDiaryView { DataContext = App.Services!.GetService(typeof(ConfirmDiaryViewModel)) });
                break;
            case PageType.GenDiary:
                _mainFrame.Navigate(new GenerateDiaryView{DataContext = App.Services!.GetService(typeof(GenerateDiaryViewModel)) });
                break;

        }
    }
}