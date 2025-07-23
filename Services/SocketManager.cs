using MalangDiary.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using MalangDiary.Structs;
using System.Text.Json;

namespace MalangDiary.Services {
    public interface ISocketManager {
        Task EnsureConnectedAsync();          // 자동 연결 (중복 방지)
        bool IsConnected { get; }
        void Send(string message);
        void Send(WorkItem item);
        WorkItem Receive();
        void Close();
    }

    public class SocketManager : ISocketManager {
        private TcpClient? client;
        private NetworkStream? stream;
        private bool isConnecting = false;
        private bool isConnected = false;

        private readonly string ip = "10.10.20.112";
        private readonly int port = 5556;

        public bool IsConnected => isConnected && client?.Connected == true;

        public async Task EnsureConnectedAsync() {
            if (IsConnected || isConnecting)
                return;

            isConnecting = true;

            client = new TcpClient();
            await client.ConnectAsync(ip, port);
            if( client.Connected is true ) {
                stream = client.GetStream();
                isConnected = true;
                Console.WriteLine($"[SocketManager] 서버 연결 완료: {ip}:{port}");
            }
            else if ( client.Connected is false ) {
                Console.WriteLine($"[SocketManager] 연결 실패");
                isConnected = false;
            }

                //try {
                //    client = new TcpClient();
                //    await client.ConnectAsync(ip, port);
                //    stream = client.GetStream();
                //    isConnected = true;
                //    Console.WriteLine($"[SocketManager] 서버 연결 완료: {ip}:{port}");
                //}
                //catch (Exception ex) {
                //    Console.WriteLine($"[SocketManager] 연결 실패: {ex.Message}");
                //    isConnected = false;
                //}
                //finally {
                //    isConnecting = false;
                //}
        }


        /// <summary>
        /// 서버로 메시지를 보냅니다.
        /// </summary>
        /// <param name="message"></param>
        public void Send(string message) {
            if (!IsConnected) {
                Console.WriteLine("[SocketManager] 연결되지 않음");
                return;
            }

            byte[] data = Encoding.UTF8.GetBytes(message);
            stream?.Write(data, 0, data.Length);
        }

        /// <summary>
        /// 서버로 WorkItem을 보냅니다.
        /// </summary>
        /// <param name="item"></param>
        public void Send(WorkItem item) {

            Console.WriteLine("Item:" + item.ToString());
            string jsonstring = JsonSerializer.Serialize(item.json);

            // JSON → 바이트
            byte[] jsonBytes = Encoding.UTF8.GetBytes(item.json);
            int jsonLen = jsonBytes.Length;

            // 파일 payload
            byte[] payloadBytes = item.payload ?? new byte[0];
            int payloadLen = payloadBytes.Length;

            // 전체 길이 계산
            int totalLen = jsonLen + payloadLen;

            // 헤더 생성 (total_len + json_len) = 8바이트
            byte[] header = new byte[8];
            Array.Copy(BitConverter.GetBytes(totalLen), 0, header, 0, 4);
            Array.Copy(BitConverter.GetBytes(jsonLen), 0, header, 4, 4);

            // 전송 (헤더 → JSON → 파일)
            stream?.Write(header, 0, 8);
            stream?.Write(jsonBytes, 0, jsonLen);
            if (payloadLen > 0)
                stream?.Write(payloadBytes, 0, payloadLen);

            Console.WriteLine("json: " + item.json);
            Console.WriteLine("payload: " + (payloadBytes.Length > 0 ? "있음" : "없음"));
            Console.WriteLine("payload 길이: " + payloadBytes.Length);
            Console.WriteLine("전체 길이: " + totalLen);

            // 한번에 담아서 보내는 법
            //List<byte> packet = new List<byte>();
            //packet.AddRange(header);
            //packet.AddRange(jsonBytes);
            //packet.AddRange(payloadBytes);

            //stream.Write(packet.ToArray(), 0, packet.Count);

        }

        /// <summary>
        /// 서버로부터 WorkItem을 수신합니다.
        /// </summary>
        /// <returns></returns>
        public WorkItem Receive() {
            byte[] header = ReadExact(8); // 헤더 8바이트 읽기

            // total_len, json_len 추출
            int totalLen = BitConverter.ToInt32(header, 0);
            int jsonLen = BitConverter.ToInt32(header, 4);
            int payloadLen = totalLen - jsonLen;

            // JSON 부분 읽기
            byte[] jsonBytes = ReadExact(jsonLen);
            string json = Encoding.UTF8.GetString(jsonBytes);

            // 파일 부분 (있다면) 읽기
            byte[] payload = payloadLen > 0 ? ReadExact(payloadLen) : new byte[0];

            Console.WriteLine("json: " + json);
            Console.WriteLine("payload: " + (payload.Length > 0 ? "있음" : "없음"));
            Console.WriteLine("payload 길이: " + payload.Length);
            Console.WriteLine("전체 길이: " + totalLen);

            return new WorkItem
            {
                json = json,
                payload = payload
            };
        }

        /// <summary>
        /// 'size' 크기만큼 정확히 데이터를 읽어옵니다.
        /// </summary>
        /// <param name="size"></param>
        /// <returns></returns>
        /// <exception cref="Exception"></exception>
        private byte[] ReadExact(int size) {
            byte[] buffer = new byte[size];
            int totalRead = 0;

            while (totalRead < size) {
                int read = 0;
                if (stream != null)
                    read = stream.Read(buffer, totalRead, size - totalRead);
                if (read == 0)
                    throw new Exception("연결 끊김 또는 데이터 부족");
                totalRead += read;
            }

            return buffer;
        }


        public void Close() {
            stream?.Close();
            client?.Close();
        }
    }
}
