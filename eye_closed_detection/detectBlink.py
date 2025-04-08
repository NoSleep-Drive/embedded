#!/usr/bin/env python
# coding: utf-8

# In[ ]:


from sklearn.cluster import KMeans
import numpy as np, dlib, cv2, imutils, time, timeit
import matplotlib.pyplot as plt
from scipy.spatial import distance as dist
from imutils.video import VideoStream
from imutils import face_utils
from threading import Thread

import sys
sys.path.append('../processing')

from removeLight import remove_light


# In[ ]:


detector = dlib.get_frontal_face_detector()
predictor = dlib.shape_predictor("shape_predictor_68_face_landmarks.dat")


# In[ ]:


(lStart, lEnd) = face_utils.FACIAL_LANDMARKS_IDXS["left_eye"]
(rStart, rEnd) = face_utils.FACIAL_LANDMARKS_IDXS["right_eye"]


# In[ ]:


def calculate_ear(eye):
    A = dist.euclidean(eye[1], eye[5])
    B = dist.euclidean(eye[2], eye[4])
    C = dist.euclidean(eye[0], eye[3])
    ear = (A + B) / (2.0 * C)
    return ear


# In[ ]:


def validate_eye_detection(rects, shape):
    if len(rects) == 0:
        raise ValueError("[ERROR] 얼굴을 감지 불가.")
    
    if shape is None or len(shape) == 0:
        raise ValueError("[ERROR] 얼굴 랜드마크 확인 불가")


# In[ ]:


def initialize_adaptive_ear(vs, frame_count=5, sensitivity=0.1):
    def measure_ear_state(is_eye_open):
        state = "뜸" if is_eye_open else "감음"
        print(f"[INFO] 눈 {state} 상태 측정을 시작")
        print(f"[INFO] 측정 중... ({state})")

        ears = []
        collected = 0
        while collected < frame_count:
            frame = vs.read()
            frame = imutils.resize(frame, width=400)
            _, gray = remove_light(frame)
            rects = detector(gray, 0)

            if len(rects) == 0:
                print("[WARN] 얼굴 미감지로 인한 예외 처리")
                time.sleep(0.05)
                continue
            
            shape = predictor(gray, rects[0])
            shape = face_utils.shape_to_np(shape)

            validate_eye_detection(rects, shape)
            
            for rect in rects:
                shape = predictor(gray, rect)
                shape = face_utils.shape_to_np(shape)
                leftEye = shape[lStart:lEnd]
                rightEye = shape[rStart:rEnd]
                leftEAR = calculate_ear(leftEye)
                rightEAR = calculate_ear(rightEye)
                ear = (leftEAR + rightEAR) / 2.0
                ears.append(ear)
                collected += 1
                print(f"[DEBUG] EAR ({state}): {ear:.3f}")
                break  # 한 명만 인식

            time.sleep(0.05)

        return sum(ears) / len(ears)

    open_ear = measure_ear_state(is_eye_open=True)
    close_ear = measure_ear_state(is_eye_open=False)

    threshold = ((open_ear - close_ear) / 2.0) + close_ear
    print(f"[INFO] 자동 설정된 EAR 기준값: {threshold:.3f} (OPEN: {open_ear:.3f}, CLOSE: {close_ear:.3f})\n")
    return threshold


# In[ ]:


def is_eye_closed(frame, threshold):
    try:
        frame = imutils.resize(frame, width=400)
        _, gray = remove_light(frame)
        rects = detector(gray, 0)

        if len(rects) == 0:
            raise ValueError("얼굴 감지 실패")

        shape = predictor(gray, rects[0])
        shape = face_utils.shape_to_np(shape)
        leftEye = shape[lStart:lEnd]
        rightEye = shape[rStart:rEnd]
        leftEAR = calculate_ear(leftEye)
        rightEAR = calculate_ear(rightEye)
        both_ear = (leftEAR + rightEAR) / 2.0
        print(f"[DEBUG] EAR: {both_ear:.3f}, Threshold: {threshold:.3f}")
        return both_ear < threshold

    except Exception as e:
        print(f"[WARN] is_eye_closed 수행 중 객체 검출 실패로 인한 오류 발생: {e}")
        return False


# In[ ]:


vs = VideoStream(src=0).start()
time.sleep(1.0)

EAR_THRESH = initialize_adaptive_ear(vs)

eye_open_counter = 0
eye_close_counter = 0
required_frames = 3 

while True:
    frame = vs.read()
    
    try:
        if is_eye_closed(frame, EAR_THRESH):
            eye_close_counter += 1
            eye_open_counter = 0
            if eye_close_counter >= required_frames:
                print("눈 감음")
        else:
            eye_open_counter += 1
            eye_close_counter = 0
            if eye_open_counter >= required_frames:
                print("눈 뜸")
    
    except Exception as e:
        print(f"[ERROR] {e}")
        continue

    cv2.imshow("Frame", frame)
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cv2.destroyAllWindows()
vs.stop()