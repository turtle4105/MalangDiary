using CommunityToolkit.Mvvm.ComponentModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MalangDiary.Models;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Messages;
using MalangDiary.Enums;
using System.IO;
using NAudio.Wave;

namespace MalangDiary.ViewModels
{
    public partial class RgsChdVoiceViewModel : ObservableObject {
    
        /* Constructor */
        public RgsChdVoiceViewModel(RgsModel rgsmodel) {
            _rgsmodel = rgsmodel;
        }
        
        private readonly RgsModel _rgsmodel;

        private bool isRecorded = false; // 저장 상태 여부

        [ObservableProperty]
        private string saveButtonText = "녹음 종료";


        private WaveInEvent? waveIn;
        private WaveFileWriter? writer;
        //private readonly string recordingPath = "C:\\Users\\yhr\\Downloads\\setting_voice.wav";
        // 1. 실제 녹음 저장 경로 (recordings 폴더에 날짜 기반으로 생성되도록 변경도 가능)
        private readonly string recordingPath = Path.Combine("recordings", "setting_voice.wav");


        /* Member Methods */
        [RelayCommand] private static void GoBack() {
            Console.WriteLine("[RgsChdVoiceViewModel] GoBack command executed.");
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        private string _recordingStatus = "듣는 중";
        public string RecordingStatus
        {
            get => _recordingStatus;
            set => SetProperty(ref _recordingStatus, value); // CommunityToolkit의 ObservableObject 방식
        }


        // ViewModel
        [RelayCommand]
        private void StopRecording()
        {
            Console.WriteLine("[ViewModel] 녹음 종료 버튼 눌림");
            StopRecordingInternal();

            isRecorded = true;
            SaveButtonText = "저장하기"; // 버튼 텍스트 변경
        }

        [RelayCommand]
        private void SaveVoice()
        {
            if (!isRecorded)
            {
                StopRecording(); // 상태가 녹음 중이라면 강제 종료
                return;
            }

            Console.WriteLine("[ViewModel] 저장하기 버튼 눌림");
            var (success, message) = _rgsmodel.SetBabyVoice(recordingPath);

            if (success)
                Console.WriteLine("서버 전송 성공!");
            else
                Console.WriteLine($"전송 실패: {message}");
        }


        [RelayCommand]
        private void RestartRecording()
        {
            Console.WriteLine("[ViewModel] 다시 녹음 버튼 눌림");
            StopRecordingInternal();
            StartRecording();

            isRecorded = false;
            SaveButtonText = "녹음 종료"; // 다시 초기 상태로
        }



        private void StartRecording()
        {
            Directory.CreateDirectory("recordings"); // recordings 폴더 생성

            waveIn = new WaveInEvent();
            waveIn.DeviceNumber = 0;
            waveIn.WaveFormat = new WaveFormat(44100, 1);

            writer = new WaveFileWriter(recordingPath, waveIn.WaveFormat);

            waveIn.DataAvailable += (s, a) => {
                writer?.Write(a.Buffer, 0, a.BytesRecorded);
            };

            waveIn.RecordingStopped += (s, a) => {
                writer?.Dispose();
                writer = null;
                waveIn.Dispose();
            };

            waveIn.StartRecording();
            RecordingStatus = "녹음 중...";
        }

        private void StopRecordingInternal()
        {
            waveIn?.StopRecording(); // 이 안에서 writer도 Dispose 됨
            RecordingStatus = "녹음 종료됨";
        }


    }
}
