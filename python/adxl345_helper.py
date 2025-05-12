import time
import board
import busio
import adafruit_adxl34x

class ADXL345Sensor:
    def __init__(self):
        self.i2c = busio.I2C(board.SCL, board.SDA)
        self.accelerometer = adafruit_adxl34x.ADXL345(self.i2c)
        self.accelerometer.enable_motion_detection(threshold=18)

    def get_acceleration(self):
        return self.accelerometer.acceleration

    def is_moving(self):
        return self.accelerometer.events['motion']
