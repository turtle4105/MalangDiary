using CommunityToolkit.Mvvm.ComponentModel;
using MalangDiary.Models;
using System;

namespace MalangDiary.ViewModels
{
    public partial class GenerateDiaryViewModel : ObservableObject
    {
        private readonly DiaryModel _diaryModel;

        [ObservableProperty] private string title = "";
        [ObservableProperty] private string text = "";
        [ObservableProperty] private string[] emotions = Array.Empty<string>();

        public GenerateDiaryViewModel(DiaryModel diaryModel)
        {
            _diaryModel = diaryModel;

            Title = _diaryModel.GetResultTitle() ?? "(제목 없음)";
            Text = _diaryModel.GetResultText() ?? "(내용 없음)";
            Emotions = _diaryModel.GetResultEmotions();
        }
    }
}
