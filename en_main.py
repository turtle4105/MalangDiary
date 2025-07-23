import os
import re
import tempfile
import logging
import json
import urllib.parse
import numpy as np
import asyncio
import time
import glob
import torch
import soundfile as sf
from datetime import datetime
from typing import List, Dict, Any
from concurrent.futures import ThreadPoolExecutor, as_completed
from fastapi import FastAPI, File, UploadFile, HTTPException, Form
from fastapi.responses import JSONResponse
from typing import Union
from openai import OpenAI
from dotenv import load_dotenv
from faster_whisper import WhisperModel
from resemblyzer import VoiceEncoder
try:
    from Levenshtein import distance as levenshtein_distance
except ImportError:
    def levenshtein_distance(s1: str, s2: str) -> int:
        """순수 Python으로 Levenshtein 거리 계산"""
        if len(s1) < len(s2):
            s1, s2 = s2, s1
        if len(s2) == 0:
            return len(s1)
        previous_row = range(len(s2) + 1)
        for i, c1 in enumerate(s1):
            current_row = [i + 1]
            for j, c2 in enumerate(s2):
                insertions = previous_row[j + 1] + 1
                deletions = current_row[j] + 1
                substitutions = previous_row[j] + (c1 != c2)
                current_row.append(min(insertions, deletions, substitutions))
            previous_row = current_row
        return previous_row[-1]
try:
    from hanspell import spell_checker  # pip install py-hanspell (오프라인 환경에선 주석 처리)
except ImportError:
    spell_checker = None

from sklearn.metrics.pairwise import cosine_similarity as sklearn_cosine_similarity

# 1. UTF-8 인코딩 설정 (Windows에서 한글 출력 문제 해결)
import sys, io
sys.stdout = io.TextIOWrapper(sys.stdout.detach(), encoding='utf-8')
sys.stderr = io.TextIOWrapper(sys.stderr.detach(), encoding='utf-8')

# 2. 로깅 설정
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('voice_server.log', encoding='utf-8')
    ]
)
logger = logging.getLogger(__name__)

step_logger = logging.getLogger('steps')
step_handler = logging.FileHandler('processing_steps.log', encoding='utf-8')
step_handler.setFormatter(logging.Formatter('%(asctime)s - %(message)s'))
step_logger.addHandler(step_handler)
step_logger.setLevel(logging.INFO)

# 3. 환경 변수 로딩
logger.info("=== 서버 초기화 시작 ===")
load_dotenv()
api_key = os.getenv("OPENAI_API_KEY")
if not api_key:
    logger.error("OPENAI_API_KEY 환경변수가 설정되지 않았습니다")
    raise ValueError("OPENAI_API_KEY is required")
client = OpenAI(api_key=api_key)
logger.info("OpenAI 클라이언트 초기화 완료")

# 4. Whisper 모델 로딩
try:
    logger.info("Whisper 모델 로딩 시작 (small, auto device, float32)")
    device = "cuda" if torch.cuda.is_available() else "cpu"
    whisper_model = WhisperModel("small", device='cuda', compute_type="int8_float32")
    logger.info(f"Whisper 모델 로딩 완료 - device: {device}")
except Exception as e:
    logger.error(f"Whisper 모델 로딩 실패: {e}")
    raise

# 5. VoiceEncoder 모델 로딩
try:
    logger.info("VoiceEncoder 모델 로딩 시작")
    voice_encoder = VoiceEncoder('cpu')
    logger.info("VoiceEncoder 모델 로딩 완료")
except Exception as e:
    logger.error(f"VoiceEncoder 모델 로딩 실패: {e}")
    raise

# 6. 아이 음성 임베딩 초기화
child_embedding = None

# 7. FastAPI 앱 초기화
app = FastAPI(title="음성인식, 정제, API")
logger.info("=== 서버 초기화 완료 ===")

# 8. 유틸리티 함수들
def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    """코사인 유사도 계산"""
    norm_a = np.linalg.norm(a)
    norm_b = np.linalg.norm(b)
    if norm_a == 0 or norm_b == 0:
        return 0.0
    return np.dot(a, b) / (norm_a * norm_b)

def process_segment_embedding(segment_audio: np.ndarray, sample_rate: int, child_embedding: np.ndarray, voice_encoder: VoiceEncoder, similarity_threshold: float = 0.6) -> float:
    """단일 세그먼트의 임베딩 처리 및 유사도 계산"""
    try:
        min_samples = int(0.5 * sample_rate)
        if len(segment_audio) < min_samples:
            padding = np.zeros(min_samples - len(segment_audio))
            segment_audio = np.concatenate([segment_audio, padding])
        
        if sample_rate != 16000:
            import librosa
            segment_audio = librosa.resample(segment_audio, orig_sr=sample_rate, target_sr=16000)
        
        embedding = voice_encoder.embed_utterance(segment_audio)
        similarity = sklearn_cosine_similarity([embedding], [child_embedding])[0][0]
        logger.debug(f"세그먼트 처리 - 길이: {len(segment_audio)} 샘플, 샘플레이트: {sample_rate}")
        return float(similarity)
    except Exception as e:
        logger.error(f"세그먼트 임베딩 처리 실패 - 길이: {len(segment_audio)}, 샘플레이트: {sample_rate}, 오류: {e}")
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
    """아이 이름을 호명하는 패턴 확인 (오타 허용)"""
    if not child_name:
        return False
    
    calling_patterns = [
        f"{child_name}야", f"{child_name}가", f"{child_name}는",
        f"{child_name}이", f"{child_name}아", f"{child_name} ",
        f"{child_name},", f"{child_name}!", f"{child_name}?"
    ]
    
    for pattern in calling_patterns:
        if pattern in text:
            return True
        for word in text.split():
            if len(word) >= len(child_name) - 1 and levenshtein_distance(word, child_name) <= 2:
                return True
    return False

def identify_child_voice_optimized(audio_path: str, segments_list: List, child_name: str = None, child_embedding_param: np.ndarray = None, similarity_threshold: float = 0.6) -> List[str]:
    """아이 음성 식별 (메모리 기반 + 병렬 처리 + 이름 호명 패턴 감지)"""
    if child_embedding_param is None:
        logger.warning("아이 음성 임베딩이 없어서 수동 필터링 사용")
        return filter_child_voice_manual(segments_list, child_name)
    
    try:
        waveform, sample_rate = torchaudio.load(audio_path)
        audio_data = waveform.squeeze(0).numpy()
        logger.info(f"오디오 파일 로딩 완료: {len(audio_data)} 샘플, {sample_rate} Hz")
        
        custom_segments = split_audio(audio_data, sample_rate, segment_duration=2)
        selected_texts = []
        similarity_results = []
        
        def process_custom_segment(seg, text_segment):
            try:
                segment_audio = seg['audio']
                similarity = process_segment_embedding(
                    segment_audio, sample_rate, child_embedding_param, voice_encoder, similarity_threshold
                )
                has_name_calling = has_child_name_calling_pattern(text_segment.text, child_name)
                return {
                    'text': text_segment.text,
                    'start': seg['start_time'],
                    'end': seg['end_time'],
                    'similarity': similarity,
                    'has_name_calling': has_name_calling,
                    'is_child': (similarity >= similarity_threshold) and not has_name_calling
                }
            except Exception as e:
                logger.error(f"세그먼트 처리 실패 ({seg['start_time']:.2f}-{seg['end_time']:.2f}): {e}")
                return {
                    'text': text_segment.text,
                    'start': seg['start_time'],
                    'end': seg['end_time'],
                    'similarity': 0.0,
                    'has_name_calling': has_child_name_calling_pattern(text_segment.text, child_name),
                    'is_child': False
                }
        
        with ThreadPoolExecutor(max_workers=4) as executor:
            futures = [executor.submit(process_custom_segment, custom_seg, text_seg) 
                      for custom_seg, text_seg in zip(custom_segments, segments_list)]
            for future in as_completed(futures):
                result = future.result()
                similarity_results.append(result)
                name_info = f" (이름호명: {result.get('has_name_calling', False)})" if child_name else ""
                logger.info(f"세그먼트 [{result['start']:.2f}-{result['end']:.2f}]: 유사도 {result['similarity']:.3f} - {'아이' if result['is_child'] else '어른'}{name_info}")
                if result['is_child']:
                    selected_texts.append(result['text'])
        
        if not selected_texts or len(selected_texts) < 5:
            logger.warning("아이 음성으로 식별된 구간이 부족해서 수동 필터링 사용")
            return filter_child_voice_manual(segments_list, child_name)
        
        selected_texts = list(dict.fromkeys(selected_texts))
        logger.info(f"아이 음성 식별 완료: {len(selected_texts)}개 텍스트 추출")
        
        similarities = [r['similarity'] for r in similarity_results]
        child_count = sum(1 for r in similarity_results if r['is_child'])
        logger.info(f"유사도 통계 - 평균: {np.mean(similarities):.3f}, 최대: {np.max(similarities):.3f}, 아이 음성: {child_count}/{len(similarity_results)}")
        
        return selected_texts
    except Exception as e:
        logger.error(f"음성 식별 실패: {e}")
        return filter_child_voice_manual(segments_list, child_name)

def filter_child_voice_manual(segments_list: List, child_name: str = None) -> List[str]:
    """수동으로 아이 음성 필터링 (범용 패턴)"""
    child_texts = []
    child_patterns = [
        r"^응$|^네$|^아니야$|^맞아$|^음$|^어$|^좋아$|^싫어$|^재미있어$|^재미없어$",
        r"했어$|^배웠어$|^갔어$|^왔어$|^먹었어$",
        r"^아팠어$|^피곤해$|^기쁨$|^슬퍼$|^화나$",
        r"^몰라$|^모르겠어$|^모르겠음$",
        r"^[가-힣]{2,6}$",
        r"게임|놀이|물놀이|방학|책|읽었어|기억|머리|이|털|다리|엄마|시간|공부|밥|잠|계획|힌트|운동|학교|집|자|미션|쉬자|탐험|국수"
    ]
    parent_patterns = [
        r".*\?$",
        r"대답|녹음|대화|해야|하면|하자|할까|해봐|야지",
        r"뭐야|뭐 했어|어떻게|왜|무슨|어디|언제|누구",
        r"크게|얘기|시간|말고|또|다른|특별한|잘|안|못",
        r"알았어|그렇구나|그럼|이유|뭐할까",
        r"^[가-힣]{7,}$"
    ]
    
    for seg in segments_list:
        text = seg.text.strip()
        if len(text) < 2:
            continue
        if child_name and has_child_name_calling_pattern(text, child_name):
            logger.debug(f"이름 호명 패턴 감지로 제외: '{text}'")
            continue
        is_parent = any(re.search(pattern, text) for pattern in parent_patterns)
        if is_parent:
            continue
        is_child = any(re.search(pattern, text) for pattern in child_patterns) or len(text) <= 4
        if is_child:
            child_texts.append(text)
    
    logger.info(f"수동 필터링 결과: {len(child_texts)}개 텍스트 추출")
    return child_texts

def remove_timestamps(text: str) -> str:
    """타임스탬프 제거"""
    logger.debug(f"타임스탬프 제거 전: {text[:100]}...")
    text = re.sub(r"\[\d{2}:\d{2}(?:\.\d{2,})?\]\s*", "", text)
    result = re.sub(r"\s+", " ", text).strip()
    logger.debug(f"타임스탬프 제거 후: {result[:100]}...")
    return result

def simple_text_cleanup(text: str, child_name: str = None) -> str:
    """단순 텍스트 정리 및 동적 오타 보정"""
    logger.info(f"단순 텍스트 정리 시작 - 입력 길이: {len(text)} 문자")
    
    # 동적 발음 유사 오타 패턴 (일부 하드코딩 대체)
    replacements = {
        "형내네": "흉내내", "형내내": "흉내내", "반과후": "방과후", 
        "까빠": "까먹어", "따가웠다": "따가웠어", "이따가": "따가",
        "놀이할 동안가": "놀이활동이", "글씨 쪄준 거": "글씨 쓰는 거",
        "타먹": "탐험", "타멈": "탐험", "탐혐": "탐험",
        "단치국수": "잔치국수", "구ㅅㅜ": "국수", "국쑤": "국수"
    }
    
    result = text
    for old, new in replacements.items():
        result = result.replace(old, new)
    
    if child_name:
        name_variations = [
            child_name[:-1] + '루', child_name[:-1] + '누', child_name[:-1] + '효',
            child_name[:-1] + '으', child_name[:-1] + '호'
        ]
        for var in name_variations:
            if var != child_name:
                result = re.sub(re.escape(var), child_name, result, flags=re.IGNORECASE)
        logger.info(f"이름 보정 적용: {child_name} 변형 치환 완료")
    
    # hanspell 맞춤법 교정 (오프라인 환경이라면 주석 처리)
    if spell_checker:
        try:
            checked = spell_checker.check(result)
            result = checked.checked
            logger.info(f"맞춤법 교정 완료: {result[:100]}...")
        except Exception as e:
            logger.warning(f"맞춤법 교정 실패: {e}")
    
    result = re.sub(r'\s+', ' ', result).strip()
    logger.info(f"단순 텍스트 정리 완료 - 출력 길이: {len(result)} 문자")
    return result

def generate_diary_with_emotions(child_text: str, full_context: str, child_name: str) -> dict:
    """아이 발화 중심 GPT 일기 생성"""
    logger.info(f"일기+감정 통합 생성 시작 - 아이 발화 길이: {len(child_text)} / 전체 맥락 길이: {len(full_context)} / 아이 이름: {child_name}")

    prompt = f"""

    이 대화는 아이와 보호자의  대화입니다. 등장 인물, 주요 사건, 대화 맥락{full_context}
, 아이의 감정{child_text}과 행동을 포함해 일기 형태로 작성해줘."

 
    이 대화는 아이와 보호자의 대화입니다. 전체 대화에서 이름과 비슷한 이름이 나오면 '{child_name}'로 보정하여 일기를 작성하세요. 등장 인물, 주요 사건, 대화 맥락{full_context}, 아이의 감정{child_text}과 행동을 포함해 아이 관점의 일기 형태로 작성해주세요

   
    아이들은 자기 자신을 3인칭 이름으로 부르지 않으므로 '{child_name}는' 같은 표현은 사용하지 않습니다.
    일기는 아이의 감정과 행동을 강조하며, 대화의 주요 사건을 반영해야 합니다.

    [전체 대화 맥락]
    {full_context}

    [아이 발화 내용]
    {child_text}

    [아이 이름]
    {child_name}

    --- 출력 형식 ---
    제목: (아이의 말에 기반한 짧은 제목, 이모지 가능)  
    내용: (아이의 말 중심으로 정돈된 일기 형식의 문단, 4~10문장 내외 - 전체 스크립트의 양에 따라 유동적)  
    
    """

    try:
        response = client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[{"role": "user", "content": prompt}],
            temperature=0.4,
        )
        response_text = response.choices[0].message.content.strip()

        try:
            tokens = response.usage.total_tokens if hasattr(response, 'usage') else 'Unknown'
            logger.info(f"통합 생성 완료 - 길이: {len(response_text)} 문자, 토큰: {tokens}")
        except Exception as log_error:
            logger.info(f"통합 생성 완료 - 길이: {len(response_text)} 문자 (토큰 로깅 실패: {log_error})")

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

@app.post("/transcribe")
async def transcribe(
    file: UploadFile = File(...), 
    embedding_file: Union[UploadFile, str] = Form(...),
    child_name: str = Form(...)
):
    request_id = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    logger.info(f"=== 새로운 요청 시작 [{request_id}] ===")
    
    child_name = urllib.parse.unquote(child_name)
    if child_name:
        logger.info(f"[{request_id}] 아이 이름: '{child_name}' - 호명 패턴 감지 활성화")
    step_logger.info(f"REQUEST {request_id}: 새로운 transcribe 요청 (아이 이름: {child_name})")
    
    tmp_audio_path = None
    tmp_embedding_path = None
    
    try:
        # 파일 검증
        logger.info(f"업로드된 오디오 파일: {file.filename}, 크기: {file.size} bytes, 타입: {file.content_type}")
        if isinstance(embedding_file, str):
            logger.info(f"업로드된 임베딩 데이터: 문자열 형태, 길이: {len(embedding_file)} 문자")
        else:
            logger.info(f"업로드된 임베딩 파일: {embedding_file.filename}, 크기: {embedding_file.size} bytes, 타입: {embedding_file.content_type}")
        
        if not file.content_type or not file.content_type.startswith('audio/'):
            logger.warning(f"잘못된 오디오 파일 타입: {file.content_type}")
            raise HTTPException(status_code=400, detail="Audio file required")
        
        if not isinstance(embedding_file, str):
            if not embedding_file.content_type or not embedding_file.content_type.startswith('application/json'):
                logger.warning(f"잘못된 임베딩 파일 타입: {embedding_file.content_type}")
                raise HTTPException(status_code=400, detail="JSON embedding file required")

        # 임시 파일 생성 (오디오)
        with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp_audio:
            audio_data = await file.read()
            tmp_audio.write(audio_data)
            tmp_audio_path = tmp_audio.name
            logger.info(f"임시 오디오 파일 생성: {tmp_audio_path}")
            step_logger.info(f"REQUEST {request_id}: 임시 오디오 파일 생성 - {tmp_audio_path}")

        # 임베딩 데이터 로딩
        try:
            if isinstance(embedding_file, str):
                embedding_str = embedding_file.strip()
                logger.info(f"[{request_id}] 문자열 임베딩 데이터 직접 수신")
            else:
                with tempfile.NamedTemporaryFile(delete=False, suffix=".json") as tmp_embedding:
                    embedding_content = await embedding_file.read()
                    tmp_embedding.write(embedding_content)
                    tmp_embedding_path = tmp_embedding.name
                    logger.info(f"임시 임베딩 파일 생성: {tmp_embedding_path}")
                with open(tmp_embedding_path, 'r', encoding='utf-8') as f:
                    embedding_str = f.read().strip()
                logger.info(f"[{request_id}] 파일 형태 임베딩 데이터 수신")
            
            embedding_values = json.loads(embedding_str)
            if not isinstance(embedding_values, list) or not all(isinstance(x, (int, float)) for x in embedding_values):
                raise ValueError("임베딩 데이터는 숫자 배열이어야 합니다")
            if len(embedding_values) != 256:
                logger.warning(f"[{request_id}] 임베딩 차원 불일치: 기대 256, 실제 {len(embedding_values)}")
            request_child_embedding = np.array(embedding_values, dtype=np.float32)
            logger.info(f"[{request_id}] 문자열 배열 임베딩 데이터 로딩 완료 - 차원: {request_child_embedding.shape}")
            step_logger.info(f"REQUEST {request_id}: 임베딩 데이터 로딩 성공")
        except (json.JSONDecodeError, ValueError) as e:
            logger.error(f"[{request_id}] 임베딩 데이터 오류: {e}, 입력: {embedding_str[:100]}...")
            request_child_embedding = None

        # STEP 1: Whisper 전사
        logger.info(f"[{request_id}] STEP 1: Whisper 전사 시작")
        step_logger.info(f"REQUEST {request_id}: STEP 1 - Whisper 전사 시작")
        
        try:
            async def async_transcribe():
                start_time = time.time()
                segments, info = await asyncio.to_thread(
                    whisper_model.transcribe, 
                    tmp_audio_path, 
                    beam_size=3,
                    vad_filter=False,
                    language="ko",
                    initial_prompt="탐험 잔치국수 국수 놀이활동 흉내내"  # 오타 방지용 프롬프트
                )
                logger.info(f"[{request_id}] Whisper internal processing time: {time.time() - start_time:.2f}초, duration: {info.duration:.2f}초")
                return segments, info
            
            segments, info = await asyncio.wait_for(async_transcribe(), timeout=300.0)
            segments_list = []
            vad_start = time.time()
            for segment in segments:
                segments_list.append(segment)
                logger.debug(f"[{request_id}] 세그먼트 duration: {(segment.end - segment.start):.2f}초")
            logger.info(f"[{request_id}] VAD & segments conversion time: {time.time() - vad_start:.2f}초")
            logger.info(f"[{request_id}] Whisper 전사 완료 - {len(segments_list)}개 세그먼트 추출")
            step_logger.info(f"REQUEST {request_id}: STEP 1 완료 - {len(segments_list)}개 세그먼트")
        except asyncio.TimeoutError:
            logger.error(f"[{request_id}] Whisper 전사 타임아웃")
            raise HTTPException(status_code=504, detail="Whisper transcription timed out")
        except Exception as e:
            logger.error(f"[{request_id}] Whisper 전사 실패: {e}")
            raise HTTPException(status_code=500, detail=f"Whisper transcription error: {str(e)}")
        
        if not segments_list:
            logger.warning(f"[{request_id}] 음성에서 텍스트를 추출할 수 없습니다")
            raise HTTPException(status_code=400, detail="No speech detected")
        
        for i, segment in enumerate(segments_list[:3]):
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
        
        full_context = " ".join([seg.text for seg in segments_list])
        logger.debug(f"[{request_id}] 정제 전 full_context: {full_context[:100]}...")
        full_context = simple_text_cleanup(full_context, child_name)
        logger.debug(f"[{request_id}] 정제 후 full_context: {full_context[:100]}...")
        logger.info(f"[{request_id}] 전체 맥락 텍스트 생성 - 길이: {len(full_context)} 문자")
        
        child_texts = identify_child_voice_optimized(tmp_audio_path, segments_list, child_name, request_child_embedding, similarity_threshold=0.6)
        child_only_text = " ".join(child_texts)
        logger.debug(f"[{request_id}] 정제 전 child_only_text: {child_only_text[:100]}...")
        child_only_text = simple_text_cleanup(child_only_text, child_name)
        logger.debug(f"[{request_id}] 정제 후 child_only_text: {child_only_text[:100]}...")
        logger.info(f"[{request_id}] 아이 음성 식별 완료 - 추출된 텍스트 수: {len(child_texts)}")
        logger.info(f"[{request_id}] 아이 음성 내용: {child_only_text}")
        step_logger.info(f"REQUEST {request_id}: STEP 3 완료 - 아이 음성: {child_only_text[:100]}...")
        
        # STEP 4: 텍스트 정제
        logger.info(f"[{request_id}] STEP 4: 텍스트 정제 단계")
        step_logger.info(f"REQUEST {request_id}: STEP 4 - 텍스트 정제 시작")
        refined_text = simple_text_cleanup(whisper_text, child_name)
        
        # STEP 5: 통합 일기+감정 생성
        logger.info(f"[{request_id}] STEP 5: 일기+감정 통합 생성 단계")
        step_logger.info(f"REQUEST {request_id}: STEP 5 - 일기+감정 통합 생성 시작")
        diary_result = generate_diary_with_emotions(child_only_text, full_context, child_name)
        
        # STEP 6: 결과 반환
        result = {
            "transcript": whisper_text,
            "emotion": ", ".join(diary_result["emotions"]),
            "title": diary_result["title"],
            "content": diary_result["content"]
        }
        
        result_filename = f"result_{request_id}.json"
        try:
            with open(result_filename, 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            for old_file in glob.glob("result_*.json"):
                if os.path.getmtime(old_file) < time.time() - 7 * 86400:
                    os.remove(old_file)
                    logger.debug(f"[{request_id}] 오래된 파일 삭제: {old_file}")
            logger.info(f"[{request_id}] 결과 파일 저장: {result_filename}")
        except Exception as e:
            logger.warning(f"[{request_id}] 결과 파일 저장 실패: {e}")
        
        logger.info(f"[{request_id}] === 요청 처리 완료 ===")
        step_logger.info(f"REQUEST {request_id}: 모든 단계 완료 - 결과 반환")
        return JSONResponse(result)
    
    except Exception as e:
        logger.error(f"[{request_id}] 요청 처리 중 오류 발생: {e}")
        step_logger.error(f"REQUEST {request_id}: 치명적 오류 - {e}")
        raise HTTPException(status_code=500, detail=f"Processing error: {str(e)}")
    
    finally:
        if tmp_audio_path and os.path.exists(tmp_audio_path):
            os.remove(tmp_audio_path)
            logger.debug(f"[{request_id}] 임시 오디오 파일 삭제: {tmp_audio_path}")
        if tmp_embedding_path and not isinstance(embedding_file, str) and os.path.exists(tmp_embedding_path):
            os.remove(tmp_embedding_path)
            logger.debug(f"[{request_id}] 임시 임베딩 파일 삭제: {tmp_embedding_path}")
        step_logger.info(f"REQUEST {request_id}: 임시 파일들 정리 완료")

if __name__ == "__main__":
    import uvicorn
    logger.info("=== 서버 시작 ===")
    uvicorn.run(app, host="0.0.0.0", port=8000)


# ### 테스트 및 확인
# 1. **패키지 설치**:
#    - `pip install python-Levenshtein py-hanspell` (오프라인이면 `hanspell` 주석 처리).
#    - 확인: `pip show python-Levenshtein py-hanspell`.
# 2. **GPU 확인**:
#    - `python -c "import torch; print(torch.cuda.is_available())"`.
# 3. **서버 실행**:
#    - `python en_main.py`.
#    - C++ 클라이언트로 요청 (child_name="전인우", audio 파일, 임베딩 문자열).
# 4. **로그 확인**:
#    - `voice_server.log`, `processing_steps.log`: `child_only_text` 정제 전/후, 세그먼트 duration(2초 내외), 오타 수정 여부.
#    - `result_{request_id}.json`: "타먹" → "탐험", "단치국수" → "잔치국수" 확인.
# 5. **Whisper 테스트**:
#    - `initial_prompt` 효과 확인: 로그에서 "탐험", "잔치국수" 포함 여부.
#    - 필요 시 "medium" 모델 주석 해제.

# ### 예상 결과
# - **오타 수정**: "타먹" → "탐험", "단치국수" → "잔치국수"로 정제.
# - **일기 개선**: GPT가 문맥 기반으로 "탐험" 올바르게 해석, "수학 시간에 동물 흉내내기" 같은 오류 감소.
# - **2초 세그먼트**: `identify_child_voice_optimized`에서 `split_audio` 적용, 로그에 duration 2초 내외 출력.

# pip install git+https://github.com/ssut/py-hanspell.git
