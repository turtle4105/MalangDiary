using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Input;

namespace MalangDiary.ViewModels {
    public class ChkCldViewModel : INotifyPropertyChanged {
        public ObservableCollection<DayModel> Days { get; set; } = new ObservableCollection<DayModel>();
        public ICommand DayClickCommand { get; set; }
        public ICommand PreviousMonthCommand { get; set; }
        public ICommand NextMonthCommand { get; set; }

        private DateTime _currentDate;
        public string CurrentMonthText => $"{_currentDate:yyyy년 M월}";

        public ChkCldViewModel() {
            Console.WriteLine("ChkCldViewModel 생성됨");

            _currentDate = DateTime.Now;
            GenerateCalendar(_currentDate);

            DayClickCommand = new RelayCommand<DayModel>(OnDayClicked);
            PreviousMonthCommand = new RelayCommand(_ =>
            {
                _currentDate = _currentDate.AddMonths(-1);
                GenerateCalendar(_currentDate);
                OnPropertyChanged(nameof(CurrentMonthText));
            });

            NextMonthCommand = new RelayCommand(_ =>
            {
                _currentDate = _currentDate.AddMonths(1);
                GenerateCalendar(_currentDate);
                OnPropertyChanged(nameof(CurrentMonthText));
            });
        }

        private void GenerateCalendar(DateTime targetDate) {
            Days.Clear();

            DateTime firstDayOfMonth = new DateTime(targetDate.Year, targetDate.Month, 1);
            int daysInMonth = DateTime.DaysInMonth(targetDate.Year, targetDate.Month);
            int skipDays = (int)firstDayOfMonth.DayOfWeek; // 일:0 ~ 토:6

            // 앞 빈 칸 추가
            for (int i = 0; i < skipDays; i++) {
                Days.Add(new DayModel { DayText = "" });
            }

            // 실제 날짜 추가
            for (int day = 1; day <= daysInMonth; day++) {
                Days.Add(new DayModel
                {
                    DayText = day.ToString(),
                    HasLike = false
                });
            }
        }

        private void OnDayClicked(DayModel day) {
            if (!string.IsNullOrEmpty(day.DayText)) {
                MessageBox.Show($"{day.DayText}일을 클릭했습니다!");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void OnPropertyChanged([CallerMemberName] string name = null) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public class DayModel {
        public string DayText { get; set; }
        public bool HasLike { get; set; }
    }
}
