import cv2

def remove_light(frame):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    lab = cv2.cvtColor(frame, cv2.COLOR_BGR2LAB)
    L = lab[:, :, 0]
    med_L = cv2.medianBlur(L, 99)
    invert_L = cv2.bitwise_not(med_L)
    composed = cv2.addWeighted(gray, 0.75, invert_L, 0.25, 0)
    return L, composed