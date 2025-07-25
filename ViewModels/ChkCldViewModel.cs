using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
    using CommunityToolkit.Mvvm.Messaging;
    using MalangDiary.Enums;
    using MalangDiary.Messages;
    using MalangDiary.Models;
    using MalangDiary.Structs;
    using System;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Runtime.CompilerServices;
    using System.Windows;
    using System.Windows.Input;

    namespace MalangDiary.ViewModels {
        public partial class ChkCldViewModel : ObservableObject, INotifyPropertyChanged {

            /** Constructor **/
            public ChkCldViewModel( ChkModel chkmodel, UserSession userSession) {
                Console.WriteLine("ChkCldViewModel 생성됨");

                _chkModel = chkmodel;
                _session = userSession;

                _currentDate = DateTime.Now;
                GenerateCalendar(_currentDate);
            }


            /** Member Variables **/
            public ObservableCollection<DayModel> Days { get; set; } = new ObservableCollection<DayModel>();

            private DateTime _currentDate;
            public string CurrentMonthText => $"{_currentDate:yyyy년 M월}";

            ChkModel _chkModel;
            UserSession _session;



            /** Member Methods **/

            [RelayCommand] private void DayClick(DayModel day) {
                Console.WriteLine("[ChkCldViewModel] DayClickCommand Executed");
                if (!string.IsNullOrEmpty(day.DayText)) {
                    _session.SetCurrentDiaryUid(day.DiaryUid);

                    WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkDiary));
                }
            }

            [RelayCommand] private void PreviousMonth() {
                _currentDate = _currentDate.AddMonths(-1);
                GenerateCalendar(_currentDate);
                OnPropertyChanged(nameof(CurrentMonthText));
            }

            [RelayCommand] private void NextMonth() {
                _currentDate = _currentDate.AddMonths(1);
                GenerateCalendar(_currentDate);
                OnPropertyChanged(nameof(CurrentMonthText));
            }


        private void GenerateCalendar(DateTime targetDate) {
            Days.Clear();

            DateTime firstDayOfMonth = new(targetDate.Year, targetDate.Month, 1);
            int daysInMonth = DateTime.DaysInMonth(targetDate.Year, targetDate.Month);
            int skipDays = (int)firstDayOfMonth.DayOfWeek;

            // [1] Get CalendarInfo using Model
            List<CalendarInfo> calendarInfoList = _chkModel.GetCalendar(targetDate);

            Dictionary<int, CalendarInfo> calendarMap = calendarInfoList
                .ToDictionary(info => info.Date, info => info);

            // [2] Add Empty Days
            for (int i = 0; i < skipDays; i++) {
                Days.Add(new DayModel { DayText = "" });
            }

            // [3] Add Actual Days
            for (int day = 1; day <= daysInMonth; day++) {
                var dayModel = new DayModel
                {
                    DayText = day.ToString()
                };

                if (calendarMap.ContainsKey(day)) {
                    var info = calendarMap[day];
                    dayModel.DiaryUid = info.Uid;
                    dayModel.Date = info.Date;
                    dayModel.IsWrited = info.IsWrited;
                    dayModel.IsLiked = info.IsLiked;
                }

                Days.Add(dayModel);
            }
        }

            public event PropertyChangedEventHandler? PropertyChanged;
            private void OnPropertyChanged([CallerMemberName] string name = null) {
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
            }

            [RelayCommand] private static void GoBack() {
                Console.WriteLine("[ChkCldViewModel] GoBack command executed.");
                WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Home));
            }

            [RelayCommand] private static void GoToChkGallery() {
                Console.WriteLine("[ChkGalleryModel] GoToChkGallery command executed.");
                WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ChkGallery));
            }
        }

    public partial class DayModel : ObservableObject {
        [ObservableProperty] private string? dayText;
        [ObservableProperty] private int diaryUid;
        [ObservableProperty] private int date;
        [ObservableProperty] private bool isWrited;
        [ObservableProperty] private bool isLiked;
    }
}
