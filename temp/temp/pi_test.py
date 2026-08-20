import serial
import threading
import time

class PI_ObservationBuffer:
    def __init__(self, history_size):
        self.history_size = history_size
        self.observation_buffer = deque(maxlen=history_size)

        PORT = "/dev/ttyACM0"
        BAUDRATE = 115200
        self.ser = serial.Serial(port=PORT,baudrate=BAUDRATE,timeout=0.1)

        self.running = True

        self.rx_thread = threading.Thread(target=self.get_observation,daemon=True)
        self.rx_thread.start()
        self.ai_thread = threading.Thread(target=self.send_trajectory,daemon=True)
        self.ai_thread.start()

    def receive_observation(self):
        while self.running:
            try:
                line = self.ser.readline()
                if line:
                    observation = self.protocol_decode(line.decode("utf-8",errors="ignore").strip())
                    # observation :{'step': ...,'q': [...],'v': [...],'current': [...]}
                    with self.lock:
                        self.observation_buffer.append(observation) # deque(maxlen=h)이므로 자동으로 최근 h개만 남는다.
            except serial.SerialException:
                break

    def send_trajectory(self):
        while self.running:
            with self.lock:
                history = list(self.observation_buffer)
            if history:
                trajectory = ai(history)
                self.ser.write(trajectory + "\n").encode("utf-8")

    def close(self):
        self.running = False
        self.ser.close()

if __name__ == "__main__":
    pi_ob = PI_ObservationBuffer()
    ai = AI()

    try:
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        pi_ob.close()