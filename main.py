import os
import re
import tempfile
import logging
import json
import numpy as np
import soundfile as sf
from datetime import datetime
from typing import List, Dict, Any
from concurrent.futures import ThreadPoolExecutor, as_completed
from fastapi import FastAPI, File, UploadFile, HTTPException, Query, Form
from fastapi.responses import JSONResponse
from typing import Union
from openai import OpenAI
from dotenv import load_dotenv
from faster_whisper import WhisperModel
from resemblyzer import VoiceEncoder
import torchaudio
from sklearn.metrics.pairwise import cosine_similarity as sklearn_cosine_similarity

# 1. 로깅 설정
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),  # 터미널 출력
        logging.FileHandler('voice_server.log', encoding='utf-8')  # 파일 출력
    ]
)
logger = logging.getLogger(__name__)

# 처리 단계별 로그 파일
step_logger = logging.getLogger('steps')
step_handler = logging.FileHandler('processing_steps.log', encoding='utf-8')
step_handler.setFormatter(logging.Formatter('%(asctime)s - %(message)s'))
step_logger.addHandler(step_handler)
step_logger.setLevel(logging.INFO)

# 2. 환경 변수 로딩
logger.info("=== 서버 초기화 시작 ===")
load_dotenv()
api_key = os.getenv("OPENAI_API_KEY")
if not api_key:
    logger.error("OPENAI_API_KEY 환경변수가 설정되지 않았습니다")
    raise ValueError("OPENAI_API_KEY is required")
client = OpenAI(api_key=api_key)
logger.info("OpenAI 클라이언트 초기화 완료")

# 3. Whisper 모델 로딩
try:
    logger.info("Whisper 모델 로딩 시작 (small, cuda, float32)")
    whisper_model = WhisperModel("small", device="cpu", compute_type="int8_float32") #int8_float32
    logger.info("Whisper 모델 로딩 완료")
except Exception as e:
    logger.error(f"Whisper 모델 로딩 실패: {e}")
    raise

# 4. VoiceEncoder 모델 로딩
try:
    logger.info("VoiceEncoder 모델 로딩 시작")
    voice_encoder = VoiceEncoder('cpu')
    logger.info("VoiceEncoder 모델 로딩 완료")
except Exception as e:
    logger.error(f"VoiceEncoder 모델 로딩 실패: {e}")
    raise

# 5. 아이 음성 임베딩 로딩 (기본값, 클라이언트에서 파일을 보낼 수도 있음)
child_embedding = None  # 클라이언트에서 받을 때까지 None으로 설정

# 6. FastAPI 앱 초기화
app = FastAPI(title="음성인식, 정제, API")
logger.info("=== 서버 초기화 완료 ===")

# 7. 유틸리티 함수들
def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    """코사인 유사도 계산 (기존 버전)"""
    norm_a = np.linalg.norm(a)
    norm_b = np.linalg.norm(b)
    if norm_a == 0 or norm_b == 0:
        return 0.0
    return np.dot(a, b) / (norm_a * norm_b)

def process_segment_embedding(segment_audio: np.ndarray, sample_rate: int, child_embedding: np.ndarray, voice_encoder: VoiceEncoder, similarity_threshold: float = 0.68) -> float:
    """단일 세그먼트의 임베딩 처리 및 유사도 계산 (메모리 기반)"""
    try:
        # 세그먼트가 너무 짧은 경우 패딩
        min_samples = int(0.5 * sample_rate)  # 최소 0.5초
        if len(segment_audio) < min_samples:
            padding = np.zeros(min_samples - len(segment_audio))
            segment_audio = np.concatenate([segment_audio, padding])
        
        # Resemblyzer용 전처리 (16kHz로 리샘플링)
        if sample_rate != 16000:
            import librosa
            segment_audio = librosa.resample(segment_audio, orig_sr=sample_rate, target_sr=16000)
        
        # VoiceEncoder로 임베딩 추출 (메모리 기반)
        embedding = voice_encoder.embed_utterance(segment_audio)
        
        # 유사도 계산 (sklearn 사용)
        similarity = sklearn_cosine_similarity([embedding], [child_embedding])[0][0]
        
        return float(similarity)
    except Exception as e:
        logger.error(f"세그먼트 임베딩 처리 실패: {e}")
        return 0.0

def split_audio(audio: np.ndarray, sample_rate: int, segment_duration: float = 2) -> List[Dict]:
    """오디오를 2초 단위로 분할"""
    segment_samples = int(sample_rate * segment_duration)
    segments = []
    for i in range(0, len(audio), segment_samples):
        start = i
        end = min(i + segment_samples, len(audio))
        segments.append({
            "audio": audio[start:end],
            "start_time": i / sample_rate,
            "end_time": end / sample_rate
        })
    return segments

def has_child_name_calling_pattern(text: str, child_name: str) -> bool:
    """아이 이름을 호명하는 패턴이 있는지 확인"""
    if not child_name:
        return False
    
    # 아이 이름 호명 패턴들 (부모가 아이를 부를 때 사용)
    calling_patterns = [
        f"{child_name}야",     # "인우야"
        f"{child_name}가",     # "인우가" 
        f"{child_name}는",     # "인우는"
        f"{child_name}이",     # "인우이"
        f"{child_name}아",     # "인우아"
        f"{child_name} ",      # "인우 " (뒤에 공백)
        f"{child_name},",      # "인우," (쉼표)
        f"{child_name}!",      # "인우!" (느낌표)
        f"{child_name}?",      # "인우?" (물음표)
    ]
    
    # 패턴 중 하나라도 포함되면 부모 음성으로 판단
    for pattern in calling_patterns:
        if pattern in text:
            return True
    
    return False

def identify_child_voice_optimized(audio_path: str, segments_list: List, child_name: str = None, child_embedding_param: np.ndarray = None, similarity_threshold: float = 0.68) -> List[str]:
    """아이의 음성 식별 (메모리 기반 + 병렬 처리 + 이름 호명 패턴 감지)"""
    if child_embedding_param is None:
        logger.warning("아이 음성 임베딩이 없어서 전체 텍스트 사용")
        return [seg.text for seg in segments_list]
    
    try:
        # torchaudio로 오디오 로딩 (더 빠름)
        waveform, sample_rate = torchaudio.load(audio_path)
        audio_data = waveform.squeeze(0).numpy()  # (channels, samples) -> (samples,)
        logger.info(f"오디오 파일 로딩 완료: {len(audio_data)} 샘플, {sample_rate} Hz")
        
        # Whisper 세그먼트를 기준으로 오디오 추출
        selected_texts = []
        similarity_results = []
        
        def process_whisper_segment(seg):
            """Whisper 세그먼트 기준으로 오디오 슬라이싱 및 임베딩 처리"""
            try:
                # 시간 기반 오디오 슬라이싱 (메모리 상에서)
                start_sample = int(seg.start * sample_rate)
                end_sample = int(seg.end * sample_rate)
                segment_audio = audio_data[start_sample:end_sample]
                
                # 임베딩 처리
                similarity = process_segment_embedding(
                    segment_audio, sample_rate, child_embedding_param, voice_encoder, similarity_threshold
                )
                
                # 아이 이름 호명 패턴 체크 (부모 음성 감지)
                has_name_calling = has_child_name_calling_pattern(seg.text, child_name)
                
                return {
                    'text': seg.text,
                    'start': seg.start,
                    'end': seg.end,
                    'similarity': similarity,
                    'has_name_calling': has_name_calling,
                    'is_child': (similarity >= similarity_threshold) and not has_name_calling
                }
            except Exception as e:
                logger.error(f"세그먼트 처리 실패 ({seg.start:.2f}-{seg.end:.2f}): {e}")
                has_name_calling = has_child_name_calling_pattern(seg.text, child_name)
                return {
                    'text': seg.text,
                    'start': seg.start,
                    'end': seg.end,
                    'similarity': 0.0,
                    'has_name_calling': has_name_calling,
                    'is_child': False
                }
        
        # 병렬 처리로 모든 세그먼트 처리
        with ThreadPoolExecutor(max_workers=4) as executor:
            futures = [executor.submit(process_whisper_segment, seg) for seg in segments_list]
            
            for future in as_completed(futures):
                result = future.result()
                similarity_results.append(result)
                
                # 로깅 (이름 호명 여부 포함)
                name_info = f" (이름호명: {result.get('has_name_calling', False)})" if child_name else ""
                logger.info(f"세그먼트 [{result['start']:.2f}-{result['end']:.2f}]: 유사도 {result['similarity']:.3f} - {'아이' if result['is_child'] else '어른'}{name_info}")
                
                # 아이 음성으로 판정된 텍스트만 추출
                if result['is_child']:
                    selected_texts.append(result['text'])
        
        if not selected_texts:
            logger.warning("아이 음성으로 식별된 구간이 없어서 전체 텍스트 사용")
            return [seg.text for seg in segments_list]
        
        # 중복 제거 (순서 유지)
        selected_texts = list(dict.fromkeys(selected_texts))
        logger.info(f"아이 음성 식별 완료: {len(selected_texts)}개 텍스트 추출")
        
        # 유사도 통계 로깅
        similarities = [r['similarity'] for r in similarity_results]
        child_count = sum(1 for r in similarity_results if r['is_child'])
        logger.info(f"유사도 통계 - 평균: {np.mean(similarities):.3f}, 최대: {np.max(similarities):.3f}, 아이 음성: {child_count}/{len(similarity_results)}")
        
        return selected_texts
        
    except Exception as e:
        logger.error(f"음성 식별 실패: {e}")
        return [seg.text for seg in segments_list]

def filter_child_voice_manual(segments_list: List, child_name: str = None) -> List[str]:
    """수동으로 아이의 음성 패턴을 필터링 (보조 방법 + 이름 호명 패턴 감지)"""
    child_texts = []
    
    # 아이의 발화로 추정되는 패턴들 (적절한 수준으로)
    child_patterns = [
        r"^응$|^네$|^아니야$|^맞아$|^음$|^어$",  # 명확한 단답형
        r"^공부했어$|^국어$|^수학$|수학, 탐험$",  # 학교 관련
        r"^따가웠어$|^따가웠다$|^아팠어$|^목$",  # 몸 상태 표현
        r"^몰라$|^모르겠어$|^모르겠다$|뭔지 모르겠다$",  # 모르겠다는 표현
        r"^요구르트$|^김치$|^잔치국수$|밥이랑",  # 음식 관련
        r"계속 동물이 계속 바뀌었어$|^동물",  # 아이다운 표현들
        r"학교에서 그거랑 수학 공부할 때$|학교에서 놀이활동이$",  # 아이의 긴 문장도 포함
    ]
    
    # 엄마 발화로 추정되는 패턴들 (더 포괄적으로 제외)
    parent_patterns = [
        r".*\?$",  # 질문문
        r"대답을|녹음이|엄마랑|대화하는|해야|하면|하자|할까|해봐",  # 지시/제안 어투  
        r"설아|서아",  # 아이 이름 호명 (기본값)
        r"뭐했어|뭐 배웠어|어떻게|왜|무슨|어디|언제",  # 질문 키워드
        r"크게|얘기|시간에|말고|또|다른|특별한|잘|안|못",  # 엄마 어투
        r"나왔어|나오지|반찬|급식|아이스크림|많이|매일",  # 음식 관련 질문
        r"일찍|자야|누워서|내일|학교|가서|제일|즐거웠어|힘들었어",  # 일상 관리
        r"알았어|잘 자|그렇구나|그럼|이유가|뭘까",  # 엄마 반응
        r"^[가-힣]{7,}$",  # 7글자 이상의 긴 문장
    ]
    
    for seg in segments_list:
        text = seg.text.strip()
        
        # 너무 짧은 텍스트 제외
        if len(text) < 2:
            continue
        
        # 아이 이름 호명 패턴 체크 (부모 음성으로 판단)
        if child_name and has_child_name_calling_pattern(text, child_name):
            logger.debug(f"이름 호명 패턴 감지로 제외: '{text}'")
            continue
            
        # 엄마 패턴 체크
        is_parent = False
        for pattern in parent_patterns:
            if re.search(pattern, text):
                is_parent = True
                break
                
        if is_parent:
            continue
            
        # 아이 패턴 체크
        is_child = False
        for pattern in child_patterns:
            if re.search(pattern, text):
                is_child = True
                break
                
        # 짧은 단답형이거나 아이 패턴에 맞으면 포함
        # 또는 매우 짧은 단답형 (응, 네, 음 등)은 무조건 포함
        if is_child or len(text) <= 2 or (len(text) <= 4 and not is_parent):
            child_texts.append(text)
    
    logger.info(f"수동 필터링 결과: {len(child_texts)}개 텍스트 추출")
    return child_texts

# 8. 타임스탬프 제거 함수
def remove_timestamps(text: str) -> str:
    logger.debug(f"타임스탬프 제거 전: {text[:100]}...")
    text = re.sub(r"\[\d{2}:\d{2}(?:\.\d{2,})?\]\s*", "", text)
    result = re.sub(r"\s+", " ", text).strip()
    logger.debug(f"타임스탬프 제거 후: {result[:100]}...")
    return result

# 6. 단순 텍스트 정리 함수 (GPT 없이)
def simple_text_cleanup(text: str) -> str:
    """GPT 없이 단순한 오타 수정만 수행"""
    logger.info(f"단순 텍스트 정리 시작 - 입력 길이: {len(text)} 문자")
    
    # 알려진 오타 패턴 수정
    replacements = {
        "형내네": "흉내내",
        "형내내": "흉내내", 
        "반과후": "방과후",
        "까빠": "까먹어",
        "따가웠다": "따가웠어",
        "이따가": "따가",
        "놀이할 동안가": "놀이활동이",
        "글씨 쪄준 거": "글씨 쓰는 거"
    }
    
    result = text
    for old, new in replacements.items():
        result = result.replace(old, new)
    
    # 불필요한 공백 정리
    result = re.sub(r'\s+', ' ', result).strip()
    
    logger.info(f"단순 텍스트 정리 완료 - 출력 길이: {len(result)} 문자")
    return result

# 7. 통합 일기 + 감정 생성 함수 (GPT 1회 호출)#- child_text: 아이가 실제로 발화한 문장들 (단답일 수 있음)

def generate_diary_with_emotions(child_text: str, full_context: str, child_name: str) -> dict:
    """
    아이 발화를 중심으로 GPT를 통해 자연스러운 일기와 감정 키워드를 생성하는 함수

    Parameters:
    - child_text: 아이가 실제로 발화한 문장들 (단답일 수 있음)
    - full_context: 보호자와 아이가 나눈 전체 대화 내용 (맥락 파악용)
    - child_name: 아이의 이름 (보호자의 발화에서 이름이 언급될 수 있음)

    Returns:
    - dict: {
        "title": 일기 제목,
        "content": 일기 본문,
        "emotions": 감정 키워드 리스트
      }
    """
    
    logger.info(f"일기+감정 통합 생성 시작 - 아이 발화 길이: {len(child_text)} / 전체 맥락 길이: {len(full_context)} / 아이 이름: {child_name}")

    # GPT에게 보낼 프롬프트 구성
    prompt = f"""
    "이 대화는 아이와 보호자의  대화입니다. 등장 인물, 주요 사건, 대화 맥락{full_context}
, 아이의 감정{child_text}과 행동을 포함해 일기 형태로 작성해줘."


[전체 대화 맥락]
{full_context}

[아이 발화 내용]
{child_text}

[아이 이름]
{child_name}

--- 출력 형식 ---
제목: (아이의 말에 기반한 짧은 제목, 이모지 가능)  
내용: (아이의 말 중심으로 정돈된 일기 형식의 문단, 10문장 내외)  
감정: (일기 내용에 기반해 유추 가능한 감정 키워드 최소 3개~5개, 쉼표로 구분)
"""

    try:
        response = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[{"role": "user", "content": prompt}],
            temperature=0.5,
        )
        response_text = response.choices[0].message.content.strip()

        # 토큰 사용량 로그
        try:
            if hasattr(response, 'usage') and response.usage:
                tokens = response.usage.total_tokens
                logger.info(f"통합 생성 완료 - 길이: {len(response_text)} 문자, 토큰: {tokens}")
            else:
                logger.info(f"통합 생성 완료 - 길이: {len(response_text)} 문자")
        except Exception as log_error:
            logger.info(f"통합 생성 완료 - 길이: {len(response_text)} 문자 (토큰 로깅 실패: {log_error})")

        # 응답에서 제목, 본문, 감정 추출
        import re
        title_match = re.search(r"제목:\s*(.*)", response_text)
        content_match = re.search(r"내용:\s*(.*?)(?=\n감정:|$)", response_text, re.DOTALL)
        emotion_match = re.search(r"감정:\s*(.*)", response_text)

        title = title_match.group(1).strip() if title_match else "일기 제목"
        content = content_match.group(1).strip() if content_match else response_text
        emotions_str = emotion_match.group(1).strip() if emotion_match else "기쁨"

        emotions = [e.strip() for e in emotions_str.split(",") if e.strip()]

        result = {
            "title": title,
            "content": content,
            "emotions": emotions
        }

        logger.info(f"일기+감정 통합 생성 완료 - 제목: {title}, 감정: {emotions}")
        return result

    except Exception as e:
        logger.error(f"일기+감정 통합 생성 실패: {e}")
        return {
            "title": "일기 생성 실패",
            "content": child_text if child_text else "내용이 없습니다.",
            "emotions": ["궁금함"]
        }

# 9. /transcribe POST API

@app.post("/transcribe")
async def transcribe(
    file: UploadFile = File(...), 
    embedding_file: Union[UploadFile, str] = Form(...),
    child_name: str = Form(...)
):
    request_id = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    logger.info(f"=== 새로운 요청 시작 [{request_id}] ===")
    if child_name:
        logger.info(f"[{request_id}] 아이 이름: '{child_name}' - 호명 패턴 감지 활성화")
    step_logger.info(f"REQUEST {request_id}: 새로운 transcribe 요청 (아이 이름: {child_name})")
    
    # 파일 검증
    logger.info(f"업로드된 오디오 파일: {file.filename}, 크기: {file.size} bytes, 타입: {file.content_type}")
    
    if isinstance(embedding_file, str):
        logger.info(f"업로드된 임베딩 데이터: 문자열 형태, 길이: {len(embedding_file)} 문자")
    else:
        logger.info(f"업로드된 임베딩 파일: {embedding_file.filename}, 크기: {embedding_file.size} bytes, 타입: {embedding_file.content_type}")
    
    if not file.content_type or not file.content_type.startswith('audio/'):
        logger.warning(f"잘못된 오디오 파일 타입: {file.content_type}")
        step_logger.error(f"REQUEST {request_id}: 잘못된 오디오 파일 타입 - {file.content_type}")
        raise HTTPException(status_code=400, detail="Audio file required")
    
    # 임베딩 파일 타입 검증 (문자열인 경우 스킵)
    if not isinstance(embedding_file, str):
        if not embedding_file.content_type or not embedding_file.content_type.startswith('application/json'):
            logger.warning(f"잘못된 임베딩 파일 타입: {embedding_file.content_type}")
            step_logger.error(f"REQUEST {request_id}: 잘못된 임베딩 파일 타입 - {embedding_file.content_type}")
            raise HTTPException(status_code=400, detail="JSON embedding file required")

    # 임시 파일 생성 (오디오)
    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp_audio:
        audio_data = await file.read()
        tmp_audio.write(audio_data)
        tmp_audio_path = tmp_audio.name
        logger.info(f"임시 오디오 파일 생성: {tmp_audio_path}")
        step_logger.info(f"REQUEST {request_id}: 임시 오디오 파일 생성 - {tmp_audio_path}")

    # 임베딩 데이터 로딩 (무조건 문자열 배열로 처리)
    try:
        if isinstance(embedding_file, str):
            # 문자열로 직접 전송된 경우
            embedding_str = embedding_file.strip()
            logger.info(f"[{request_id}] 문자열 임베딩 데이터 직접 수신")
        else:
            # UploadFile로 전송된 경우
            embedding_content = await embedding_file.read()
            embedding_str = embedding_content.decode('utf-8').strip()
            logger.info(f"[{request_id}] 파일 형태 임베딩 데이터 수신")
        
        # 문자열 배열 형태로 파싱
        # 대괄호 제거 후 쉼표로 분할
        embedding_str = embedding_str.strip('[]')
        embedding_values = [float(x.strip()) for x in embedding_str.split(',') if x.strip()]
        request_child_embedding = np.array(embedding_values)
        
        logger.info(f"[{request_id}] 문자열 배열 임베딩 데이터 로딩 완료 - 차원: {request_child_embedding.shape}")
        step_logger.info(f"REQUEST {request_id}: 임베딩 데이터 로딩 성공")
    except Exception as e:
        logger.error(f"[{request_id}] 임베딩 데이터 로딩 실패: {e}")
        step_logger.error(f"REQUEST {request_id}: 임베딩 데이터 로딩 실패 - {e}")
        request_child_embedding = None

    try:
        # STEP 1: Whisper 전사
        logger.info(f"[{request_id}] STEP 1: Whisper 전사 시작")
        step_logger.info(f"REQUEST {request_id}: STEP 1 - Whisper 전사 시작")
        
        segments, _ = whisper_model.transcribe(tmp_audio_path, beam_size=3,vad_filter=True, language="ko")
        segments_list = list(segments)

        
        logger.info(f"[{request_id}] Whisper 전사 완료 - {len(segments_list)}개 세그먼트 추출")
        step_logger.info(f"REQUEST {request_id}: STEP 1 완료 - {len(segments_list)}개 세그먼트")
        
        if not segments_list:
            logger.warning(f"[{request_id}] 음성에서 텍스트를 추출할 수 없습니다")
            step_logger.error(f"REQUEST {request_id}: STEP 1 실패 - 텍스트 추출 실패")
            raise HTTPException(status_code=400, detail="No speech detected")
        
        # 세그먼트 정보 로깅
        for i, segment in enumerate(segments_list[:3]):  # 처음 3개만 로깅
            logger.debug(f"[{request_id}] 세그먼트 {i+1}: [{segment.start:.2f}-{segment.end:.2f}] {segment.text[:50]}...")
        
        raw_text = " ".join([f"[{s.start:.2f}] {s.text}" for s in segments_list])
        logger.info(f"[{request_id}] 원본 텍스트 생성 - 길이: {len(raw_text)} 문자")
        step_logger.info(f"REQUEST {request_id}: 원본 텍스트 생성 - {raw_text[:100]}...")
        
        # STEP 2: 타임스탬프 제거
        logger.info(f"[{request_id}] STEP 2: 타임스탬프 제거")
        step_logger.info(f"REQUEST {request_id}: STEP 2 - 타임스탬프 제거")
        whisper_text = remove_timestamps(raw_text)
        logger.info(f"[{request_id}] 타임스탬프 제거 완료 - 정제된 길이: {len(whisper_text)} 문자")
        step_logger.info(f"REQUEST {request_id}: STEP 2 완료 - 정제된 텍스트: {whisper_text[:100]}...")

        # STEP 3: 세그먼트 분할 및 음성 식별
        logger.info(f"[{request_id}] STEP 3: 음성 식별 단계")
        step_logger.info(f"REQUEST {request_id}: STEP 3 - 음성 식별 시작")
        
        # 전체 맥락 이해를 위한 전체 텍스트 생성
        full_context = " ".join([seg.text for seg in segments_list])
        logger.info(f"[{request_id}] 전체 맥락 텍스트 생성 - 길이: {len(full_context)} 문자")
        
        # 아이의 음성만 식별 (최적화된 메모리 기반 방식 + 이름 호명 패턴 감지)
        if child_name:
            logger.info(f"[{request_id}] 아이 이름 설정: '{child_name}' - 호명 패턴 감지 활성화")
        child_texts = identify_child_voice_optimized(tmp_audio_path, segments_list, child_name, request_child_embedding, similarity_threshold=0.68)
        child_only_text = " ".join(child_texts)
        
        logger.info(f"[{request_id}] 아이 음성 식별 완료 - 추출된 텍스트 수: {len(child_texts)}")
        logger.info(f"[{request_id}] 아이 음성 내용: {child_only_text}")
        step_logger.info(f"REQUEST {request_id}: STEP 3 완료 - 아이 음성: {child_only_text[:100]}...")
        
        # STEP 4: 텍스트 정제 (단순 오타 수정만)
        logger.info(f"[{request_id}] STEP 4: 텍스트 정제 단계 (오타 수정만)")
        step_logger.info(f"REQUEST {request_id}: STEP 4 - 텍스트 정제 시작")
        refined_text = simple_text_cleanup(whisper_text)

        # STEP 5: 통합 일기+감정 생성 (GPT 1회 호출)
        if child_name:
            logger.info(f"[{request_id}] STEP 5: 일기+감정 통합 생성 단계")
            step_logger.info(f"REQUEST {request_id}: STEP 5 - 일기+감정 통합 생성 시작")
            diary_result = generate_diary_with_emotions(child_only_text, full_context,child_name)
        else:
            logger.info(f"[{request_id}] STEP 5: 일기+감정 통합 생성 단계")
            step_logger.info(f"REQUEST {request_id}: STEP 5 - 일기+감정 통합 생성 시작")
            diary_result = generate_diary_with_emotions(child_only_text, full_context)

        # STEP 6: 결과 반환 (기존 포맷 호환)
        result = {
            "transcript": whisper_text,  # 전체 원본 텍스트 반환 (타임스탬프 제거된)
            "refined_transcript": refined_text,  # GPT로 정제된 텍스트 (오타 수정만)
            "child_voice_only": child_only_text,  # 아이 발화만 추출된 텍스트
            "emotion": ", ".join(diary_result["emotions"]),  # 기존 포맷: 문자열
            "diary": {  # 기존 포맷: 객체
                "title": diary_result["title"],
                "content": diary_result["content"]
            }
        }
        
        logger.info(f"[{request_id}] === 요청 처리 완료 ===")
        step_logger.info(f"REQUEST {request_id}: 모든 단계 완료 - 결과 반환")
        
        # 결과를 JSON 파일로도 저장
        result_filename = f"result_{request_id}.json"
        try:
            with open(result_filename, 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            logger.info(f"[{request_id}] 결과 파일 저장: {result_filename}")
        except Exception as e:
            logger.warning(f"[{request_id}] 결과 파일 저장 실패: {e}")
        
        return JSONResponse(result)

    except Exception as e:
        logger.error(f"[{request_id}] 요청 처리 중 오류 발생: {e}")
        step_logger.error(f"REQUEST {request_id}: 치명적 오류 - {e}")
        raise HTTPException(status_code=500, detail=f"Processing error: {str(e)}")
    
    finally:
        # 임시 파일 정리
        if os.path.exists(tmp_audio_path):
            os.remove(tmp_audio_path)
            logger.debug(f"[{request_id}] 임시 오디오 파일 삭제: {tmp_audio_path}")
        # 임베딩 파일이 실제 파일인 경우에만 삭제 (문자열인 경우에는 임시 파일이 없음)
        if not isinstance(embedding_file, str) and 'tmp_embedding_path' in locals() and os.path.exists(tmp_embedding_path):
            os.remove(tmp_embedding_path)
            logger.debug(f"[{request_id}] 임시 임베딩 파일 삭제: {tmp_embedding_path}")
        step_logger.info(f"REQUEST {request_id}: 임시 파일들 정리 완료")

# 서버 시작 시 로깅
if __name__ == "__main__":
    import uvicorn
    logger.info("=== 서버 시작 ===")
    uvicorn.run(app, host="0.0.0.0", port=8000)