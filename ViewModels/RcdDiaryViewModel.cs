using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using NAudio.Wave;
using System;
using System.IO;

namespace MalangDiary.ViewModels
{
    public partial class RcdDiaryViewModel : ObservableObject
    {
        public RcdDiaryViewModel(DiaryModel diarymodel)
        {
            _diarymodel = diarymodel;
        }

        private readonly DiaryModel _diarymodel;

        private WaveInEvent? waveIn;
        private WaveFileWriter? writer;

        private readonly string recordingPath = Path.Combine("recordings", "diary_voice.wav");

        [ObservableProperty]
        private string recordingStatus = "듣는 중";

        [ObservableProperty]
        private string stopButtonText = "녹음 종료";

        private bool isRecording = false;

        /* 뒤로가기 */
        [RelayCommand]
        private void GoBack()
        {
            Console.WriteLine("[RcdDiaryViewModel] GoBack command executed.");
            StopRecordingInternal();  // 페이지 나갈 때도 안전하게 정리
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        /* 다시 녹음 */
        [RelayCommand]
        private void ReRecord()
        {
            Console.WriteLine("[RcdDiaryViewModel] 다시 녹음 선택됨");
            StopRecordingInternal();
            StartRecording();

            StopButtonText = "녹음 종료";
            RecordingStatus = "녹음 중...";
            isRecording = true;
        }

        /* 녹음 종료 후 다음 페이지 이동 */
        [RelayCommand]
        private void GoToConfirmDiary()
        {
            Console.WriteLine("[RcdDiaryViewModel] 일기 확인 페이지로 이동합니다.");

            if (isRecording)
            {
                StopRecordingInternal();
                StopButtonText = "녹음 완료됨";
            }

            // 나중에 서버 전송용 path를 DiaryModel에 넘기기 위해 저장
            _diarymodel.SetDiaryVoicePath(recordingPath);

            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ConfirmDiary));
        }

        // 녹음 시작
        public void StartRecording()
        {
            Directory.CreateDirectory("recordings");

            waveIn = new WaveInEvent();
            waveIn.DeviceNumber = 0;
            waveIn.WaveFormat = new WaveFormat(44100, 1);

            writer = new WaveFileWriter(recordingPath, waveIn.WaveFormat);

            waveIn.DataAvailable += (s, a) =>
            {
                writer?.Write(a.Buffer, 0, a.BytesRecorded);
            };

            waveIn.RecordingStopped += (s, a) =>
            {
                writer?.Dispose();
                writer = null;
                waveIn.Dispose();
            };

            waveIn.StartRecording();
            RecordingStatus = "녹음 중...";
            isRecording = true;

            Console.WriteLine("[RcdDiaryViewModel] 녹음 시작됨");
        }

        // 녹음 종료 내부 처리
        private void StopRecordingInternal()
        {
            waveIn?.StopRecording();
            RecordingStatus = "녹음 종료됨";
            isRecording = false;

            Console.WriteLine("[RcdDiaryViewModel] 녹음 종료됨");
        }
    }
}
