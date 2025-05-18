#!/usr/bin/env python
# coding: utf-8

import numpy as np
import dlib
import cv2
import imutils
from scipy.spatial import distance as dist
from imutils import face_utils

import os
# Disable Qt plugin loading
os.environ["QT_QPA_PLATFORM"] = "offscreen"
# Additional settings to prevent OpenCV from using GUI
os.environ["OPENCV_VIDEOIO_PRIORITY_MSMF"] = "0"

# Initialize detector and predictor as global variables
detector = dlib.get_frontal_face_detector()
predictor = None

# 얼굴 랜드마크 인덱스
(lStart, lEnd) = face_utils.FACIAL_LANDMARKS_IDXS["left_eye"]
(rStart, rEnd) = face_utils.FACIAL_LANDMARKS_IDXS["right_eye"]

def initialize():
    """Initialize dlib face predictor"""
    global predictor
    try:
        # Load face landmark model
        predictor = dlib.shape_predictor("shape_predictor_68_face_landmarks.dat")
        return True
    except Exception as e:
        print(f"[ERROR] Error during initialization: {e}")
        return False

def calculate_ear(eye):
    """Calculate eye aspect ratio (EAR)"""
    A = dist.euclidean(eye[1], eye[5])
    B = dist.euclidean(eye[2], eye[4])
    C = dist.euclidean(eye[0], eye[3])
    ear = (A + B) / (2.0 * C)
    return ear

def is_eye_closed(frame, threshold=0.25):
    """
    Determine if eyes are closed in the input frame
    
    Parameters:
    frame (numpy.ndarray): OpenCV image frame
    threshold (float): EAR threshold, default is 0.25
    
    Returns:
    bool: True if eyes are closed, False if open
    """
    global predictor
    
    if predictor is None:
        if not initialize():
            print("[ERROR] Face predictor has not been initialized")
            return False
    
    try:
        # Resize image
        frame = imutils.resize(frame, width=400)
        
        # Detect faces
        rects = detector(frame, 0)
        
        if len(rects) == 0:
            print("[WARN] Failed to detect face")
            return False
        
        # Detect face landmarks
        shape = predictor(frame, rects[0])
        shape = face_utils.shape_to_np(shape)
        
        # Extract left/right eye landmarks
        leftEye = shape[lStart:lEnd]
        rightEye = shape[rStart:rEnd]
        
        # Calculate EAR
        leftEAR = calculate_ear(leftEye)
        rightEAR = calculate_ear(rightEye)
        both_ear = (leftEAR + rightEAR) / 2.0
        
        print(f"[DEBUG] EAR: {both_ear:.3f}, Threshold: {threshold:.3f}")
        
        # Compare with threshold to determine if eyes are closed
        return both_ear < threshold
        
    except Exception as e:
        print(f"[ERROR] Error occurred while determining eye closure: {e}")
        return False