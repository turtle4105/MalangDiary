using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using CommunityToolkit.Mvvm.Messaging;
using MalangDiary.Enums;
using MalangDiary.Messages;
using MalangDiary.Models;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.IO;

namespace MalangDiary.ViewModels
{
    public partial class RcdDiaryViewModel : ObservableObject
    {
        public RcdDiaryViewModel(DiaryModel diarymodel, BaseModel baseModel)
        {
            _diarymodel = diarymodel;
            _baseModel = baseModel;

            StartRecordButtonText = "녹음 시작";
        }

        private readonly DiaryModel _diarymodel;
        private readonly BaseModel _baseModel;

        private WaveInEvent? waveIn;
        private MemoryStream? memoryStream;  // 메모리에 버퍼 저장
        private WaveFileWriter? writer;


        private readonly string recordingPath = Path.Combine("recordings", "diary_voice.wav");

        [ObservableProperty]
        private string recordingStatus = "듣는 중";

        [ObservableProperty]
        private string stopButtonText = "녹음 종료";

        [ObservableProperty]
        private string startRecordButtonText;

        private bool isRecording = false;

        [RelayCommand]
        private void GoBack()
        {
            Console.WriteLine("[RcdDiaryViewModel] GoBack command executed.");
            StopRecordingInternal();
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.Goback));
        }

        [RelayCommand]
        private void StartRecord()
        {
            Console.WriteLine("[RcdDiaryViewModel] 녹음 시작 또는 다시 녹음 선택됨");

            StopRecordingInternal();
            StartRecording();

            StopButtonText = "녹음 종료";
            StartRecordButtonText = "다시 녹음";
            RecordingStatus = "녹음 중...";
            isRecording = true;
        }

        [RelayCommand]
        private void GoToConfirmDiary()
        {
            Console.WriteLine("[RcdDiaryViewModel] 일기 확인 페이지로 이동합니다.");

            if (isRecording)
            {
                StopRecordingInternal();
                StopButtonText = "녹음 완료됨";
            }

            _diarymodel.SetDiaryVoicePath(recordingPath);
            WeakReferenceMessenger.Default.Send(new PageChangeMessage(PageType.ConfirmDiary));
        }

        public void StartRecording()
        {
            string dirPath = "recordings";
            string fileName = "diary_voice.wav";

            memoryStream = new MemoryStream();

            waveIn = new WaveInEvent
            {
                DeviceNumber = 0,
                WaveFormat = new WaveFormat(44100, 1)
            };

            writer = new WaveFileWriter(new IgnoreDisposeStream(memoryStream), waveIn.WaveFormat);

            waveIn.DataAvailable += (s, a) =>
            {
                Console.WriteLine($"[RcdDiaryViewModel] DataAvailable: {a.BytesRecorded} bytes");
                writer?.Write(a.Buffer, 0, a.BytesRecorded);
            };

            waveIn.RecordingStopped += (s, a) =>
            {
                writer?.Flush();
                writer?.Dispose();
                writer = null;
                waveIn.Dispose();

                // Save wav from memory using BaseModel
                var bytes = memoryStream?.ToArray() ?? Array.Empty<byte>();
                bool saved = _baseModel.WriteWavToFile(dirPath, fileName, bytes);
                Console.WriteLine(saved ? "[녹음 결과] 파일 저장 성공!" : "[녹음 결과] 파일 저장 실패");

                memoryStream?.Dispose();
                memoryStream = null;
            };

            // 디바이스 목록 출력
            for (int i = 0; i < WaveIn.DeviceCount; i++)
            {
                var info = WaveIn.GetCapabilities(i);
                Console.WriteLine($"[Device {i}] {info.ProductName}");
            }

            waveIn.StartRecording();
            RecordingStatus = "녹음 중...";
            isRecording = true;

            Console.WriteLine("[RcdDiaryViewModel] 녹음 시작됨");
        }

        private void StopRecordingInternal()
        {
            waveIn?.StopRecording();
            isRecording = false;
            RecordingStatus = "녹음 종료됨";

            Console.WriteLine("[RcdDiaryViewModel] 녹음 종료됨");
        }

        // 메모리 스트림을 Dispose하지 않도록 감싸는 클래스
        private class IgnoreDisposeStream : Stream
        {
            private readonly Stream _inner;
            public IgnoreDisposeStream(Stream inner) => _inner = inner;

            public override bool CanRead => _inner.CanRead;
            public override bool CanSeek => _inner.CanSeek;
            public override bool CanWrite => _inner.CanWrite;
            public override long Length => _inner.Length;
            public override long Position { get => _inner.Position; set => _inner.Position = value; }

            public override void Flush() => _inner.Flush();
            public override int Read(byte[] buffer, int offset, int count) => _inner.Read(buffer, offset, count);
            public override long Seek(long offset, SeekOrigin origin) => _inner.Seek(offset, origin);
            public override void SetLength(long value) => _inner.SetLength(value);
            public override void Write(byte[] buffer, int offset, int count) => _inner.Write(buffer, offset, count);

            public override void Close() => _inner.Close();
            protected override void Dispose(bool disposing) { /* 무시 */ }
        }
    }
}
