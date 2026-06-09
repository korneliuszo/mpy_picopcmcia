import os
from PIL import Image
import cv2
import time

out = open("ba.raw","wb")
cap = cv2.VideoCapture("badapple.mp4")
fps = cap.get(cv2.CAP_PROP_FPS)
while True:
    ret, frame = cap.read()
    if ret == False:
        break
    color_coverted = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    i = Image.fromarray(color_coverted)
    i=i.resize((640,200))
    i=i.convert("1")#,dither=Image.Dither.FLOYDSTEINBERG)
    re=bytearray(100*80)
    ro=bytearray(100*80)
    im = i.tobytes()
    for y in range(200):
        for x in range(0,640,8):
            if(y%2):
                ro[(y//2)*80+x//8]=(im[y*(640//8)+x//8])
            else:
                re[(y//2)*80+x//8]=(im[y*(640//8)+x//8])
    
    out.write(re)
    out.write(ro)

