using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.IO;
using NAudio.Wave;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdVoiceViewModel : ObservableObject
    {
        public RgsChdVoiceViewModel(RgsModel rgsmodel)
        {
            _rgsmodel = rgsmodel;
            SaveButtonText = "녹음 시작";
            RecordingStatus = "대기 중...";
        }

        private readonly RgsModel _rgsmodel;
        private readonly string recordingPath = Path.Combine("recordings", "setting_voice.wav");

        private WaveInEvent? waveIn;
        private WaveFileWriter? writer;

        private bool isRecording = false;
        private bool isRecorded = false;

        [ObservableProperty] private string saveButtonText;
        [ObservableProperty] private string recordingStatus;

        [RelayCommand]
        private static void GoBack()
        {
            Console.WriteLine("[RgsChdVoiceViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        [RelayCommand]
        private void ToggleRecording()
        {
            if (!isRecording && !isRecorded)
            {
                // 녹음 시작
                Console.WriteLine("[ViewModel] 녹음 시작");
                StartRecording();
                SaveButtonText = "녹음 종료";
                RecordingStatus = "녹음 중...";
                isRecording = true;
            }
            else if (isRecording)
            {
                // 녹음 중 → 녹음 종료
                Console.WriteLine("[ViewModel] 녹음 종료");
                StopRecordingInternal();
                SaveButtonText = "저장하기";
                RecordingStatus = "녹음 완료됨";
                isRecording = false;
                isRecorded = true;
            }
            else if (isRecorded)
            {
                // 저장
                Console.WriteLine("[ViewModel] 저장하기");
                WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.RgsChd));
                WeakReferenceMessenger.Default.Send(new VoiceRecordedMessage(true));
            }
        }

        [RelayCommand]
        private void RestartRecording()
        {
            Console.WriteLine("[ViewModel] 다시 녹음");
            StopRecordingInternal();
            StartRecording();
            SaveButtonText = "녹음 종료";
            RecordingStatus = "녹음 중...";
            isRecording = true;
            isRecorded = false;
        }

        private void StartRecording()
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
        }

        private void StopRecordingInternal()
        {
            waveIn?.StopRecording();
            RecordingStatus = "녹음 종료됨";
        }
    }
}
