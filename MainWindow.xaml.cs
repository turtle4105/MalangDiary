using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using MalangDiary.ViewModels;

namespace MalangDiary
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        public MainWindow() {
            InitializeComponent();

            // 명시적 타입 지정
            if (App.Services!.GetService(typeof(MainViewModel)) is MainViewModel mainVM) {  // null 체크, MainViewModel 인스턴스 가져오기
                DataContext = mainVM;
                mainVM.SetFrame(MainFrame);  // MainFrame 
            }
        }
    }
}