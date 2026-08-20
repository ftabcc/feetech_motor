import serial
class ESP_system:
    def __init__(self):
        self.min_execute_term = 10 #[ms]
        self.mcu_ser = serial.Serial(port="/dev/ttyACM0",baudrate=115200,timeout=0.1)
        self.front_motor_ser = serial.Serial(port="/dev/ttyACM1",baudrate=115200,timeout=0.1)
        self.back_motor_ser = serial.Serial(port="/dev/ttyACM2",baudrate=115200,timeout=0.1)
        self.imu_ser = serial.Serial(port="/dev/ttyACM3",baudrate=115200,bytesize=8,parity=serial.PARITY_NONE,stopbits=1,timeout=0.1)
    
    def pi_rx(self):
        