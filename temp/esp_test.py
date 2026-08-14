import numpy as np
class ESP_Main:
    '''ESP32는 자신의 고정된 시간축을 기준으로 trajectory를 실행하며,
    새로운 AI trajectory(t시점 관측에 대하여 t+2~t+2+n까지 n개의 궤적)가 도착하면 현재 시점에서
    2 step 이후의 미래부터 기존 trajectory를 새 trajectory로 교체한다.
    '''
    def __init__(self):
        self.mcu_ser = serial.Serial(port="/dev/ttyACM0",baudrate=115200,timeout=0.1)
        self.front_motor_ser = serial.Serial(port="/dev/ttyACM1",baudrate=115200,timeout=0.1)
        self.back_motor_ser = serial.Serial(port="/dev/ttyACM2",baudrate=115200,timeout=0.1)
        self.imu_ser = serial.Serial(port="/dev/ttyACM3",baudrate=115200,bytesize=8,parity=serial.PARITY_NONE,stopbits=1,timeout=0.1)

        self.ai_term = 200 # AI trajectory waypoint term [ms]
        self.motor_term = 20 # motor control term [ms]

        self.trajectory = [] # list of dictionary. trajectory's each step 예: {'step': ...,'q': [...],'v': [...],'current': [...]}
        self.imu_quaternian = []
        
        self.start_time = time.monotonic()
        self.running = True

        self.get_trajectory_thread = threading.Thread(target=self.get_trajectory,daemon=True)
        self.get_trajectory_thread.start()
        self.set_trajectory_thread = threading.Thread(target=self.set_trajectory,daemon=True)
        self.set_trajectory_thread.start()
        self.observation_thread = threading.Thread(target=self.send_observation,daemon=True)
        self.observation_thread.start()
        self.imu_thread = threading.Thread(target=self.get_imu,daemon=True)
        self.imu_thread.start()

        self.lock = threading.Lock()
        self.condition = threading.Condition(self.lock)

    def stop(self):
        with self.condition:
            self.running = False
            self.condition.notify_all()

        if threading.current_thread() is not self.rx_thread:
            self.rx_thread.join()
        if threading.current_thread() is not self.tx_thread:
            self.tx_thread.join()
        if threading.current_thread() is not self.imu_thread:
            self.imu_thread.join()

        self.mcu_ser.close()
        self.front_motor_ser.close()
        self.back_motor_ser.close()
        self.imu_ser.close()

    def get_trajectory(self):
        while self.running:
            try:
                line = self.mcu_ser.readline()
                if line:
                    # trajectory : list of dictionary, each one: {'step': ...,'q': [...],'v': [...],'current': [...]}
                    new_trajectory = self.mcu_protocol(line.decode("utf-8",errors="ignore").strip()) # need to change about binary packet

                    with self.condition:
                        self.current_step = int((time.monotonic() - self.start_time) / self.ai_term) # about ai_term
                        first_update_step = self.current_step + 2 # include

                        if len(self.trajectory) > 0:
                            current_trajectory_start_step = self.trajectory[0]['step']
                            current_trajectory_last_step = self.trajectory[-1]['step']

                            if current_trajectory_last_step < self.current_step + 1: # 현재 step이 기존 trajectory 넘어서 진행된다면 err로 간주
                                print('behavior ended') # trajectory insufficient err
                                self.running = False
                                self.condition.notify_all()

                            else: # 현재 step이 기존 trajectory 내에 있어서 기존 trajectory와 새 trajectory 바로 이어지거나, 일부가 겹칠때 새 trajectory 업데이트
                                '''# INVARIANT:
                                # 정상 패킷은 연속된 step을 가지며, 패킷 순서가 보장된다.
                                # 정상 패킷의 trajectory는 current_step + 2를 반드시 포함한다.
                                #
                                # Therefore:
                                #     new_trajectory[0]['step'] <= first_update_step
                                #     <= new_trajectory[-1]['step']
                                #
                                # Consequently:
                                #     0 <= new_trajectory_update_idx < len(new_trajectory)
                                #
                                # If this condition is violated, the received trajectory is considered invalid
                                # rather than a normal trajectory update case.'''
                                new_trajectory_update_idx = first_update_step - new_trajectory[0]['step']
                                current_trajectory_update_idx = first_update_step - current_trajectory_start_step
                                self.trajectory = self.trajectory[:current_trajectory_update_idx] + new_trajectory[new_trajectory_update_idx:]
                                self.condition.notify()
                        else:
                            self.trajectory = new_trajectory
                            self.start_time = time.monotonic()
                            self.condition.notify()
                            print('new behavior started')

            except serial.SerialException:
                break


    def set_trajectory(self):
        while self.running:
            with self.condition:
                while len(self.trajectory) < 2 and self.running:
                    self.condition.wait()

                if not self.running:
                    break
                p0 = np.array([self.trajectory[0]['q'],self.trajectory[0]['v'],self.trajectory[0]['a']]).T
                p1 = np.array([self.trajectory[1]['q'],self.trajectory[1]['v'],self.trajectory[1]['a']]).T
                execute_time = self.start_time + self.trajectory[0]['step'] * self.ai_term

            waypoints = self.quintic_hermite(p0,p1,step=int(self.ai_term / self.motor_term),term=self.ai_term)

            for i in range(len(waypoints)): # exclude start point, include end point
                target_time = execute_time + (i+1) * self.motor_term
                sleep_time = target_time - time.monotonic()
                if sleep_time > 0:
                    time.sleep(sleep_time)
                self.set_motor(waypoints[i])

            with self.condition:
                if len(self.trajectory) > 1:
                    self.trajectory.pop(0)

    def send_observation(self):
        next_time = self.start_time + self.ai_term
        while self.running:
            sleep_time = next_time - time.monotonic()
            if sleep_time > 0:
                time.sleep(sleep_time)
            if not self.running:
                break
            observation = self.get_observation() # need to change. sleep -> obs or obs->sleep
            # self.send_observation(observation)
            next_time += self.ai_term
    
    def get_imu(self):
        while self.running:
            while True: # SOP 탐색
                data = self.ser.read(2)
                if len(data) != 2:
                    continue
                if data == b'\x55\x55':
                    break

            data = self.ser.read(10) #[Qz Qx Qy Qw CHK] each 2 btye
            if len(data) != 10:
                continue
            
            # Checksum
            packet = b'\x55\x55' + data
            checksum = sum(packet[:-2]) & 0xFFFF
            received_checksum = int.from_bytes(packet[-2:],byteorder='big',signed=False)
            if checksum != received_checksum:
                continue

            # 16-bit signed big-endian 변환
            q_raw = [
                int.from_bytes(packet[2:4], byteorder='big', signed=True),
                int.from_bytes(packet[4:6], byteorder='big', signed=True),
                int.from_bytes(packet[6:8], byteorder='big', signed=True),
                int.from_bytes(packet[8:10], byteorder='big', signed=True)]
            
            with self.lock:
                self.imu_quaternian = q_raw * 0.0001 # hex,quaternion 모드에서 10000으로 나눠야함. 각 항목별 소수점 아래 4자리정확도
            
    def get_observation(self):
        pass

    def set_motor(self):
        pass

    def get_motor(self):
        pass

    def quintic_hermite(self, p0, p1, step, term):
        p0 = np.asarray(p0)
        p1 = np.asarray(p1)

        # State matrix: shape (6, dof)
        P = np.column_stack([
            p0[:, 0], term * p0[:, 1], (term**2) * p0[:, 2],
            p1[:, 0], term * p1[:, 1], (term**2) * p1[:, 2]
        ]).T

        # Coefficient matrix for quintic Hermite basis
        M = np.array([
            [  1.0,   0.0,   0.0,   0.0,   0.0,   0.0],
            [  0.0,   1.0,   0.0,   0.0,   0.0,   0.0],
            [  0.0,   0.0,   0.5,   0.0,   0.0,   0.0],
            [-10.0,  -6.0,  -1.5,  10.0,  -4.0,   0.5],
            [ 15.0,   8.0,   1.5, -15.0,   7.0,  -1.0],
            [ -6.0,  -3.0,  -0.5,   6.0,  -3.0,   0.5]])

        u = np.linspace(0, 1, step + 1)[1:]
        u2 = u**2
        u3 = u**3
        u4 = u**4
        u5 = u**5

        # Monomial basis matrices: shape (step, 6)
        U   = np.column_stack([np.ones_like(u), u, u2, u3, u4, u5])
        dU  = np.column_stack([np.zeros_like(u), np.ones_like(u), 2*u, 3*u2, 4*u3, 5*u4])
        ddU = np.column_stack([np.zeros_like(u), np.zeros_like(u), 2*np.ones_like(u), 6*u, 12*u2, 20*u3])

        # Basis evaluation
        H   = U @ M
        dH  = dU @ M
        ddH = ddU @ M

        # Compute trajectory components
        q = H @ P
        v = (dH @ P) / term
        a = (ddH @ P) / (term**2)

        # Result shape: (step, dof, 3)
        return np.stack((q, v, a), axis=2)

if __name__ == "__main__":
    esp_main = ESP_Main()
    ai = AI()

    try:

    except KeyboardInterrupt:
        esp_main.stop()