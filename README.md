# MalangDiary - AI 음성 기반 어린이 일기 생성 플랫폼

<p align="center">
  <img src="./images/malang_logo.png" alt="logo" width="300" height="200"/>
</p>


## 
본 프로젝트는 어린아이의 짧고 불완전한 말을 인공지능을 통해 의미 있는 순간으로 기록하는
음성 기반 일기 생성 시스템 입니다.
보호자와 아이의 음성 파일을 토대로 아이의 발화 구간만을 선별하여 텍스트로 변환하고 ,
이를 자연스러운 일기 문장으로 생성함으로써 아이의 하루를 기록하고 되새길수 있는 환경을 제공합니다.

## 기능
- 음성 인식 (Whisper 기반)
- 감정 분석 및 요약 (ChatGPT or 감정 모델)
- 자동 일기 작성

## 설치방법
1. python 서버 구동방법
python 3.10이하
pip  install -r requirements.txt  - 의존성 설치

python main.py < nul - 서버구동  -  faster-whisper  cdnn관련 에러발생시 cpu로 구동


2. c++ 서버 구동방법
vcpkg install curl:x64-windows -vcpakg를 이용해 libcurl 설치
vcpkg integrate install  - visual studio와 연결 



## 사용 방법
1. 음성 파일 `.wav`를 업로드합니다, 아이의 목소리를 녹음합니다. 
2. 자동으로 텍스트 + 감정 분석 → 일기 생성
3. 일기 리스트를 보거나 삭제할수 있습니다.

## 브랜치
Client_dev - 클라이언트.
Server_dev - c++ 서버.
AI_dev_copy - python 서버.
