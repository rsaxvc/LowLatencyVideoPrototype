width = 640
height = 120

import cv2
import numpy as np
import time

while True:
	image = np.zeros((height,width,3), np.uint8)
	cv2.putText(image,str(time.time()), (50,50), cv2.FONT_HERSHEY_SIMPLEX, 2, 255)
	cv2.imshow('timer',image)
	cv2.waitKey(1)
