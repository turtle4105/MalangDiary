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
        Task EnsureConnectedAsync();
        bool IsConnected { get; }
        void Send(string message);
        void Send(WorkItem item);
        WorkItem Receive();
        void Close();
    }

    public class SocketManager : ISocketManager {

        /** Member Variables **/
        private TcpClient? client;
        private NetworkStream? stream;
        private bool isConnecting = false;
        private bool isConnected = false;

        private readonly string ip = "10.10.20.112";
        private readonly int port = 5556;

        public bool IsConnected => isConnected && client?.Connected == true;



        /** Member Methods **/

        /* Check Wether Connect to Server or Not */
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


        /* Send byte[] to Server */
        public void Send(string message) {
            if (!IsConnected) {
                Console.WriteLine("[SocketManager] 연결되지 않음");
                return;
            }

            byte[] data = Encoding.UTF8.GetBytes(message);
            stream?.Write(data, 0, data.Length);
        }


        /* Send header + json + byte[] to Server */
        public void Send(WorkItem item) {

            Console.WriteLine("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
            Console.WriteLine("[SocketManager] Executed public void Send(WorkItem item)");
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
            Console.WriteLine(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

        }


        /* Recevie Header + json + byte[] form Server */
        public WorkItem Receive() {
            Console.WriteLine("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
            Console.WriteLine("[SocketManager] Executed public WorkItem Receive()");
            
            byte[] header = ReadExact(8); // 헤더 8바이트 읽기
            Console.WriteLine("Read header");

            // total_len, json_len 추출
            int totalLen = BitConverter.ToInt32(header, 0);
            int jsonLen = BitConverter.ToInt32(header, 4);
            int payloadLen = totalLen - jsonLen;
            Console.WriteLine("totalLen:" + totalLen);
            Console.WriteLine("jsonLen:" + jsonLen);
            Console.WriteLine("payloadLen:" + payloadLen);

            // JSON 부분 읽기
            byte[] jsonBytes = ReadExact(jsonLen);
            string json = Encoding.UTF8.GetString(jsonBytes);
            Console.WriteLine("Read json");

            // 파일 부분 (있다면) 읽기
            byte[] payload = payloadLen > 0 ? ReadExact(payloadLen) : new byte[0];
            Console.WriteLine("Read Payload");

            Console.WriteLine("json: " + json);
            Console.WriteLine("payload: " + (payload.Length > 0 ? "있음" : "없음"));
            Console.WriteLine("payload 길이: " + payload.Length);
            Console.WriteLine("전체 길이: " + totalLen);

            Console.WriteLine("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

            return new WorkItem
            {
                json = json,
                payload = payload
            };
        }

        /* Read Size of Bytes Exactly */
        private byte[] ReadExact(int size) {
            Console.WriteLine("[ReadExact] size:" + size);
            byte[] buffer = new byte[size];
            
            int totalRead = 0;

            while (totalRead < size) {
                int read = 0;
                if (stream != null)
                    read = stream.Read(buffer, totalRead, size - totalRead);
                if (read == 0)
                    throw new Exception("연결 끊김 또는 데이터 부족");
                totalRead += read;
                Console.WriteLine("[ReadExact] read:" + read);
                Console.WriteLine("[ReadExact] totalRead:" + totalRead);
            }

            return buffer;
        }


        public void Close() {
            stream?.Close();
            client?.Close();
        }
    }
}
