from picamera import PiCamera
from time import sleep
camera = PiCamera()
camera.resolution = (800, 600)
camera.exposure_mode = 'auto'
camera.awb_mode = 'auto'
#camera.shutter_speed = 5000000
camera.start_preview()
sleep(0.1)
camera.capture('/home/pi/WasteSorting/WasteSorting.jpg', use_video_port = 'true')
camera.stop_preview()